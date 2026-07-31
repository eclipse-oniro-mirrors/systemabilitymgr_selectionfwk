/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SELECTION_GEOMETRY_UTILS_H
#define SELECTION_GEOMETRY_UTILS_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace OHOS {
namespace SelectionFwk {

struct SelectionPoint {
    int32_t x = 0;
    int32_t y = 0;

    SelectionPoint() = default;
    SelectionPoint(int32_t px, int32_t py) : x(px), y(py) {}
};

struct SelectionRect {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;

    SelectionRect() = default;
    SelectionRect(int32_t px, int32_t py, int32_t pw, int32_t ph)
        : x(px), y(py), width(pw), height(ph) {}

    bool IsEmpty() const;
    bool IsNormalized() const;
    int64_t Area() const;
    bool operator==(const SelectionRect &other) const;
    bool operator!=(const SelectionRect &other) const;
};

class SelectionGeometryUtils {
public:
    static int32_t ClampInt(int32_t value, int32_t lo, int32_t hi);
    static int32_t ClampToInt32(int64_t value);
    static int32_t SnapToPixel(double value);
    static int32_t SafeAddInt32(int32_t a, int32_t b);
    static int32_t SafeSubInt32(int32_t a, int32_t b);
    static int64_t SafeMulInt64(int32_t a, int32_t b);

    static SelectionRect MakeEmptyRect();
    static SelectionRect NormalizeRect(const SelectionRect &rect);
    static int64_t RightEdge(const SelectionRect &rect);
    static int64_t BottomEdge(const SelectionRect &rect);
    static int64_t CenterX(const SelectionRect &rect);
    static int64_t CenterY(const SelectionRect &rect);
    static int64_t Perimeter(const SelectionRect &rect);
    static double AspectRatio(const SelectionRect &rect);

    static bool ContainsPoint(const SelectionRect &rect, const SelectionPoint &point);
    static bool ContainsPoint(const SelectionRect &rect, int32_t px, int32_t py);
    static bool ContainsRect(const SelectionRect &outer, const SelectionRect &inner);
    static bool Intersects(const SelectionRect &a, const SelectionRect &b);
    static bool EqualsTolerance(const SelectionRect &a, const SelectionRect &b, int32_t tol);

    static SelectionRect IntersectRect(const SelectionRect &a, const SelectionRect &b);
    static SelectionRect UnionRect(const SelectionRect &a, const SelectionRect &b);
    static std::vector<SelectionRect> SubtractRect(const SelectionRect &a, const SelectionRect &b);
    static std::vector<SelectionRect> IntersectListWithRect(const std::vector<SelectionRect> &list,
        const SelectionRect &rect);
    static SelectionRect UnionListBounds(const std::vector<SelectionRect> &list);

    static double DistancePointRect(const SelectionPoint &point, const SelectionRect &rect);
    static int32_t SignedDistanceXPointRect(const SelectionPoint &point, const SelectionRect &rect);
    static int32_t SignedDistanceYPointRect(const SelectionPoint &point, const SelectionRect &rect);
    static double DistanceRectRect(const SelectionRect &a, const SelectionRect &b);
    static int32_t HorizontalGap(const SelectionRect &a, const SelectionRect &b);
    static int32_t VerticalGap(const SelectionRect &a, const SelectionRect &b);

    static bool IsAdjacent(const SelectionRect &a, const SelectionRect &b, int32_t tol);
    static bool IsHorizontallyAligned(const SelectionRect &a, const SelectionRect &b, int32_t tol);
    static bool IsVerticallyAligned(const SelectionRect &a, const SelectionRect &b, int32_t tol);
    static bool AreOnSameRow(const SelectionRect &a, const SelectionRect &b, int32_t tol);
    static bool AreOnSameColumn(const SelectionRect &a, const SelectionRect &b, int32_t tol);

    static SelectionRect Inflate(const SelectionRect &rect, int32_t left, int32_t top,
        int32_t right, int32_t bottom);
    static SelectionRect InflateUniform(const SelectionRect &rect, int32_t amount);
    static SelectionRect Deflate(const SelectionRect &rect, int32_t left, int32_t top,
        int32_t right, int32_t bottom);
    static SelectionRect DeflateUniform(const SelectionRect &rect, int32_t amount);
    static SelectionRect Offset(const SelectionRect &rect, int32_t dx, int32_t dy);
    static SelectionRect Scale(const SelectionRect &rect, double sx, double sy);
    static SelectionRect Transpose(const SelectionRect &rect);
    static SelectionRect MirrorHorizontal(const SelectionRect &rect, int32_t axisX);
    static SelectionRect MirrorVertical(const SelectionRect &rect, int32_t axisY);
    static SelectionRect Rotate90(const SelectionRect &rect, const SelectionPoint &pivot);

    static std::vector<SelectionRect> MergeOverlapping(const std::vector<SelectionRect> &list,
        int32_t tol);
    static int64_t TotalArea(const std::vector<SelectionRect> &list);
    static SelectionRect BoundsOfList(const std::vector<SelectionRect> &list);
    static bool ContainsAny(const std::vector<SelectionRect> &list, const SelectionRect &rect);
    static bool ContainsAll(const std::vector<SelectionRect> &list, const SelectionRect &rect);
    static std::vector<SelectionRect> RemoveContained(const std::vector<SelectionRect> &list);
    static std::vector<SelectionRect> Deduplicate(const std::vector<SelectionRect> &list, int32_t tol);

    static std::vector<std::vector<SelectionRect>> GroupByRows(const std::vector<SelectionRect> &list,
        int32_t rowTol);
    static std::vector<SelectionRect> SortByRow(const std::vector<SelectionRect> &list);

    static SelectionRect ClipToBounds(const SelectionRect &rect, const SelectionRect &bounds);
    static std::vector<SelectionRect> ClipRectListToBounds(const std::vector<SelectionRect> &list,
        const SelectionRect &bounds);

    static bool ClipSegmentToRect(SelectionPoint &p1, SelectionPoint &p2, const SelectionRect &rect);
    static bool SegmentIntersectsRect(const SelectionPoint &p1, const SelectionPoint &p2,
        const SelectionRect &rect);

    static std::vector<SelectionPoint> ClipPolygonToRect(const std::vector<SelectionPoint> &polygon,
        const SelectionRect &rect);

    static int64_t UnionArea(const std::vector<SelectionRect> &list);

    static SelectionRect MapFromParent(const SelectionRect &rect, const SelectionPoint &parentOrigin);
    static SelectionRect MapToParent(const SelectionRect &rect, const SelectionPoint &parentOrigin);
    static SelectionRect NormalizeForRTL(const SelectionRect &rect, int32_t parentWidth);

    static SelectionRect MinRectByArea(const SelectionRect &a, const SelectionRect &b);
    static SelectionRect MaxRectByArea(const SelectionRect &a, const SelectionRect &b);

    static double PointDistance(const SelectionPoint &a, const SelectionPoint &b);
    static bool PointEqualsTolerance(const SelectionPoint &a, const SelectionPoint &b, int32_t tol);
    static SelectionPoint PointLerp(const SelectionPoint &a, const SelectionPoint &b, double t);
    static SelectionPoint PointTranslate(const SelectionPoint &pt, int32_t dx, int32_t dy);
    static SelectionPoint PointRotate90(const SelectionPoint &pt, const SelectionPoint &pivot);
    static SelectionPoint PointMirrorHorizontal(const SelectionPoint &pt, int32_t axisX);
    static SelectionPoint PointMirrorVertical(const SelectionPoint &pt, int32_t axisY);
    static double SegmentLength(const SelectionPoint &a, const SelectionPoint &b);
    static double PointToSegmentDistance(const SelectionPoint &p, const SelectionPoint &s,
        const SelectionPoint &e);
    static SelectionPoint FootOfPerpendicular(const SelectionPoint &p, const SelectionPoint &s,
        const SelectionPoint &e);
    static bool IsPointOnSegment(const SelectionPoint &p, const SelectionPoint &s,
        const SelectionPoint &e);
    static std::vector<SelectionPoint> RectCorners(const SelectionRect &rect);
    static std::vector<SelectionPoint> RectEdgeMidpoints(const SelectionRect &rect);
    static int32_t ClosestEdgeIndex(const SelectionRect &rect, const SelectionPoint &point);
    static SelectionRect ExpandToContain(const SelectionRect &rect, const SelectionPoint &point);
    static SelectionRect ShrinkToAspectRatio(const SelectionRect &rect, double targetRatio);
    static std::vector<SelectionRect> SplitGrid(const SelectionRect &rect, int32_t rows, int32_t cols);
    static bool IsSquare(const SelectionRect &rect);
    static int32_t LargerAxis(const SelectionRect &rect);
    static int32_t SmallerAxis(const SelectionRect &rect);
    static SelectionRect CenterAt(const SelectionRect &rect, const SelectionPoint &center);
};

class SelectionRectCombiner {
public:
    SelectionRectCombiner() = default;
    ~SelectionRectCombiner() = default;

    void Add(const SelectionRect &rect);
    std::vector<SelectionRect> Result() const;
    void Clear();
    size_t Size() const;
    bool Empty() const;

private:
    void TryMergeAdjacent();

    std::vector<SelectionRect> rects_;
};
} // namespace SelectionFwk
} // namespace OHOS
#endif // SELECTION_GEOMETRY_UTILS_H
