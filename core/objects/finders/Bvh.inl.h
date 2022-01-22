#include "objects/finders/Bvh.h"

NAMESPACE_SPH_BEGIN

struct BvhTraversal {
    Size idx;
    Float t_min;
};

struct BvhBuildEntry {
    Size parent;
    Size start;
    Size end;
};

bool intersectBox(const Box& box, const RaySegment& ray, Interval& segment) {
    StaticArray<Vector, 2> b = { box.lower(), box.upper() };
    Float tmin = (b[ray.signs[X]][X] - ray.orig[X]) * ray.invDir[X];
    Float tmax = (b[1 - ray.signs[X]][X] - ray.orig[X]) * ray.invDir[X];
    SPH_ASSERT(!std::isnan(tmin) && !std::isnan(tmax)); // they may be inf though
    const Float tymin = (b[ray.signs[Y]][Y] - ray.orig[Y]) * ray.invDir[Y];
    const Float tymax = (b[1 - ray.signs[Y]][Y] - ray.orig[Y]) * ray.invDir[Y];
    SPH_ASSERT(!std::isnan(tymin) && !std::isnan(tymax));

    if ((tmin > tymax) || (tymin > tmax)) {
        return false;
    }
    tmin = max(tmin, tymin);
    tmax = min(tmax, tymax);

    const Float tzmin = (b[ray.signs[Z]][Z] - ray.orig[Z]) * ray.invDir[Z];
    const Float tzmax = (b[1 - ray.signs[Z]][Z] - ray.orig[Z]) * ray.invDir[Z];
    SPH_ASSERT(!std::isnan(tzmin) && !std::isnan(tzmax));

    if ((tmin > tzmax) || (tzmin > tmax)) {
        return false;
    }
    tmin = max(tmin, tzmin);
    tmax = min(tmax, tzmax);

    segment = Interval(tmin, tmax);

    // check intersections in parameter
    return segment.intersects(ray.segment());
}

template <typename TBvhObject>
template <typename TAddIntersection>
void Bvh<TBvhObject>::getIntersections(const RaySegment& ray, const TAddIntersection& addIntersection) const {
    Size closer;
    Size other;

    StaticArray<BvhTraversal, 64> stack;
    int stackIdx = 0;

    stack[stackIdx].idx = 0;
    stack[stackIdx].t_min = ray.segment().lower();

    while (stackIdx >= 0) {
        const Size idx = stack[stackIdx].idx;
        stackIdx--;
        const BvhNode& node = nodes[idx];

        /// \todo optimization for the single intersection case
        if (node.rightOffset == 0) {
            // leaf
            for (Size primIdx = 0; primIdx < node.primCnt; ++primIdx) {
                IntersectionInfo current;

                const TBvhObject& obj = objects[node.start + primIdx];
                const bool hit = obj.getIntersection(ray, current);

                if (hit) {
                    SPH_ASSERT(current.t >= 0.f); // objects should not give us intersections at t<0
                    const bool doContinue = addIntersection(current);
                    if (!doContinue) {
                        return;
                    }
                }
            }
        } else {
            // inner node
            Interval segmentLeft, segmentRight;
            const bool hitc0 = intersectBox(nodes[idx + 1].box, ray, segmentLeft);
            const bool hitc1 = intersectBox(nodes[idx + node.rightOffset].box, ray, segmentRight);

            if (hitc0 && hitc1) {
                closer = idx + 1;
                other = idx + node.rightOffset;

                if (segmentRight.lower() < segmentLeft.lower()) {
                    std::swap(segmentRight, segmentLeft);
                    std::swap(closer, other);
                }

                stack[++stackIdx] = BvhTraversal{ other, segmentRight.lower() };
                stack[++stackIdx] = BvhTraversal{ closer, segmentLeft.lower() };
            } else if (hitc0) {
                stack[++stackIdx] = BvhTraversal{ idx + 1, segmentLeft.lower() };
            } else if (hitc1) {
                stack[++stackIdx] = BvhTraversal{ idx + node.rightOffset, segmentRight.lower() };
            }
        }
    }
}

template <typename TBvhObject>
bool Bvh<TBvhObject>::getFirstIntersection(const RaySegment& ray, IntersectionInfo& intersection) const {
    intersection.t = INFTY;
    intersection.object = nullptr;

    this->getIntersections(ray, [&intersection](IntersectionInfo& current) {
        if (current.t < intersection.t) {
            intersection = current;
        }
        return true;
    });
    return intersection.object != nullptr;
}

template <typename TBvhObject>
template <typename TOutIter>
Size Bvh<TBvhObject>::getAllIntersections(const RaySegment& ray, TOutIter iter) const {
    Size count = 0;
    this->getIntersections(ray, [&iter, &count](IntersectionInfo& current) { //
        *iter = current;
        ++iter;
        ++count;
        return true;
    });
    return count;
}

template <typename TBvhObject>
bool Bvh<TBvhObject>::isOccluded(const RaySegment& ray) const {
    bool occluded = false;
    this->getIntersections(ray, [&occluded](IntersectionInfo&) {
        occluded = true;
        return false; // do not continue with traversal
    });
    return occluded;
}

template <typename TBvhObject>
void Bvh<TBvhObject>::build(Array<TBvhObject>&& objs) {
    objects = std::move(objs);
    SPH_ASSERT(!objects.empty());
    nodeCnt = 0;
    leafCnt = 0;

    StaticArray<BvhBuildEntry, 128> stack;
    Size stackIdx = 0;
    constexpr Size UNTOUCHED = 0xffffffff;
    constexpr Size NO_PARENT = 0xfffffffc;
    constexpr Size TOUCHED_TWICE = 0xfffffffd;

    // Push the root
    stack[stackIdx].start = 0;
    stack[stackIdx].end = objects.size();
    stack[stackIdx].parent = NO_PARENT;
    stackIdx++;

    BvhNode node;
    Array<BvhNode> buildNodes;
    buildNodes.reserve(2 * objects.size());

    while (stackIdx > 0) {
        BvhBuildEntry& nodeEntry = stack[--stackIdx];
        const Size start = nodeEntry.start;
        const Size end = nodeEntry.end;
        const Size primCnt = end - start;

        nodeCnt++;
        node.start = start;
        node.primCnt = primCnt;
        node.rightOffset = UNTOUCHED;

        Box bbox = objects[start].getBBox();
        const Vector center = objects[start].getCenter();
        Box boxCenter(center, center);
        for (Size i = start + 1; i < end; ++i) {
            bbox.extend(objects[i].getBBox());
            boxCenter.extend(objects[i].getCenter());
        }
        node.box = bbox;

        if (primCnt <= leafSize) {
            node.rightOffset = 0;
            leafCnt++;
        }
        buildNodes.push(node);

        if (nodeEntry.parent != NO_PARENT) {
            buildNodes[nodeEntry.parent].rightOffset--;

            if (buildNodes[nodeEntry.parent].rightOffset == TOUCHED_TWICE) {
                buildNodes[nodeEntry.parent].rightOffset = nodeCnt - 1 - nodeEntry.parent;
            }
        }

        if (node.rightOffset == 0) {
            continue;
        }

        const Size splitDim = argMax(boxCenter.size());
        const Float split = 0.5_f * (boxCenter.lower()[splitDim] + boxCenter.upper()[splitDim]);

        Size mid = start;
        for (Size i = start; i < end; ++i) {
            if (objects[i].getCenter()[splitDim] < split) {
                std::swap(objects[i], objects[mid]);
                ++mid;
            }
        }

        if (mid == start || mid == end) {
            mid = start + (end - start) / 2;
        }

        stack[stackIdx++] = { nodeCnt - 1, mid, end };
        stack[stackIdx++] = { nodeCnt - 1, start, mid };
    }

    SPH_ASSERT(buildNodes.size() == nodeCnt);
    nodes = std::move(buildNodes);
}

template <typename TBvhObject>
Box Bvh<TBvhObject>::getBoundingBox() const {
    return nodes[0].box;
}

NAMESPACE_SPH_END
