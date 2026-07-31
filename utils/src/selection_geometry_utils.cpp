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

#include "selection_geometry_utils.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

namespace OHOS {
namespace SelectionFwk {

namespace {
constexpr int32_t INT32_MIN_VALUE = std::numeric_limits<int32_t>::min();
constexpr int32_t INT32_MAX_VALUE = std::numeric_limits<int32_t>::max();

constexpr size_t MIN_POLYGON_VERTICES = 3;
constexpr int32_t RECT_EDGE_COUNT = 4;
constexpr int32_t Y_BOUNDS_PER_RECT = 2;
constexpr int64_t HALF_FACTOR = 2;
constexpr int64_t PERIMETER_FACTOR = 2;
constexpr int64_t MIRROR_FACTOR = 2;
constexpr double ROUND_HALF = 0.5;
constexpr double CLIP_T_MIN = 0.0;
constexpr double CLIP_T_MAX = 1.0;

constexpr int EDGE_LEFT = 0;
constexpr int EDGE_RIGHT = 1;
constexpr int EDGE_TOP = 2;
constexpr int EDGE_BOTTOM = 3;

int64_t AbsInt64(int64_t value)
{
    return value < 0 ? -value : value;
}

struct Interval {
    int64_t start;
    int64_t end;
};

bool OverlapOrAdjacent1D(int64_t a0, int64_t a1, int64_t b0, int64_t b1, int32_t tol)
{
    if (a0 > a1) {
        std::swap(a0, a1);
    }
    if (b0 > b1) {
        std::swap(b0, b1);
    }
    int64_t tol64 = static_cast<int64_t>(tol);
    if (tol64 < 0) {
        tol64 = 0;
    }
    return !(a1 + tol64 < b0 || b1 + tol64 < a0);
}

bool Overlap1D(int64_t a0, int64_t a1, int64_t b0, int64_t b1)
{
    if (a0 > a1) {
        std::swap(a0, a1);
    }
    if (b0 > b1) {
        std::swap(b0, b1);
    }
    return !(a1 <= b0 || b1 <= a0);
}

SelectionRect ClampRect(int64_t x, int64_t y, int64_t width, int64_t height)
{
    int64_t nx = x;
    int64_t ny = y;
    int64_t nw = width;
    int64_t nh = height;
    if (nw < 0) {
        nx += nw;
        nw = -nw;
    }
    if (nh < 0) {
        ny += nh;
        nh = -nh;
    }
    int64_t left = nx;
    int64_t top = ny;
    int64_t right = nx + nw;
    int64_t bottom = ny + nh;
    if (left < INT32_MIN_VALUE) {
        left = INT32_MIN_VALUE;
    }
    if (left > INT32_MAX_VALUE) {
        left = INT32_MAX_VALUE;
    }
    if (top < INT32_MIN_VALUE) {
        top = INT32_MIN_VALUE;
    }
    if (top > INT32_MAX_VALUE) {
        top = INT32_MAX_VALUE;
    }
    if (right < INT32_MIN_VALUE) {
        right = INT32_MIN_VALUE;
    }
    if (right > INT32_MAX_VALUE) {
        right = INT32_MAX_VALUE;
    }
    if (bottom < INT32_MIN_VALUE) {
        bottom = INT32_MIN_VALUE;
    }
    if (bottom > INT32_MAX_VALUE) {
        bottom = INT32_MAX_VALUE;
    }
    if (right < left) {
        right = left;
    }
    if (bottom < top) {
        bottom = top;
    }
    return SelectionRect(static_cast<int32_t>(left), static_cast<int32_t>(top),
        static_cast<int32_t>(right - left), static_cast<int32_t>(bottom - top));
}

bool PolygonInsidePoint(const SelectionPoint &pt, int edge, const SelectionRect &bounds)
{
    int64_t x = pt.x;
    int64_t y = pt.y;
    int64_t l = bounds.x;
    int64_t r = SelectionGeometryUtils::RightEdge(bounds);
    int64_t t = bounds.y;
    int64_t b = SelectionGeometryUtils::BottomEdge(bounds);
    switch (edge) {
        case EDGE_LEFT:
            return x >= l;
        case EDGE_RIGHT:
            return x < r;
        case EDGE_TOP:
            return y >= t;
        case EDGE_BOTTOM:
            return y < b;
        default:
            return false;
    }
}

SelectionPoint PolygonEdgeIntersect(const SelectionPoint &s, const SelectionPoint &e,
    int edge, const SelectionRect &bounds)
{
    int64_t x1 = s.x;
    int64_t y1 = s.y;
    int64_t x2 = e.x;
    int64_t y2 = e.y;
    int64_t dx = x2 - x1;
    int64_t dy = y2 - y1;
    int64_t l = bounds.x;
    int64_t r = SelectionGeometryUtils::RightEdge(bounds);
    int64_t t = bounds.y;
    int64_t b = SelectionGeometryUtils::BottomEdge(bounds);
    switch (edge) {
        case EDGE_LEFT: {
            if (dx == 0) {
                return SelectionPoint(static_cast<int32_t>(l), static_cast<int32_t>(y1));
            }
            double tt = static_cast<double>(l - x1) / static_cast<double>(dx);
            int64_t ny = y1 + static_cast<int64_t>(tt * static_cast<double>(dy));
            return SelectionPoint(static_cast<int32_t>(l), static_cast<int32_t>(ny));
        }
        case EDGE_RIGHT: {
            if (dx == 0) {
                return SelectionPoint(static_cast<int32_t>(r), static_cast<int32_t>(y1));
            }
            double tt = static_cast<double>(r - x1) / static_cast<double>(dx);
            int64_t ny = y1 + static_cast<int64_t>(tt * static_cast<double>(dy));
            return SelectionPoint(static_cast<int32_t>(r), static_cast<int32_t>(ny));
        }
        case EDGE_TOP: {
            if (dy == 0) {
                return SelectionPoint(static_cast<int32_t>(x1), static_cast<int32_t>(t));
            }
            double tt = static_cast<double>(t - y1) / static_cast<double>(dy);
            int64_t nx = x1 + static_cast<int64_t>(tt * static_cast<double>(dx));
            return SelectionPoint(static_cast<int32_t>(nx), static_cast<int32_t>(t));
        }
        case EDGE_BOTTOM: {
            if (dy == 0) {
                return SelectionPoint(static_cast<int32_t>(x1), static_cast<int32_t>(b));
            }
            double tt = static_cast<double>(b - y1) / static_cast<double>(dy);
            int64_t nx = x1 + static_cast<int64_t>(tt * static_cast<double>(dx));
            return SelectionPoint(static_cast<int32_t>(nx), static_cast<int32_t>(b));
        }
        default:
            return e;
    }
}

std::vector<SelectionPoint> ClipPolygonSingleEdge(const std::vector<SelectionPoint> &input,
    int edge, const SelectionRect &bounds)
{
    std::vector<SelectionPoint> output;
    size_t n = input.size();
    if (n == 0) {
        return output;
    }
    for (size_t i = 0; i < n; ++i) {
        SelectionPoint cur = input[i];
        SelectionPoint prev = input[(i + n - 1) % n];
        bool curIn = PolygonInsidePoint(cur, edge, bounds);
        bool prevIn = PolygonInsidePoint(prev, edge, bounds);
        if (curIn) {
            if (!prevIn) {
                output.push_back(PolygonEdgeIntersect(prev, cur, edge, bounds));
            }
            output.push_back(cur);
        } else if (prevIn) {
            output.push_back(PolygonEdgeIntersect(prev, cur, edge, bounds));
        }
    }
    return output;
}

bool ApplySegmentClipEdge(double p, double q, double &t0, double &t1)
{
    if (p == 0.0) {
        return q >= 0.0;
    }
    double t = q / p;
    if (p < 0.0) {
        if (t > t1) {
            return false;
        }
        if (t > t0) {
            t0 = t;
        }
    } else {
        if (t < t0) {
            return false;
        }
        if (t < t1) {
            t1 = t;
        }
    }
    return true;
}

int64_t MergeIntervalLengths(std::vector<Interval> &intervals)
{
    if (intervals.empty()) {
        return 0;
    }
    std::sort(intervals.begin(), intervals.end(),
        [](const Interval &a, const Interval &b) { return a.start < b.start; });
    int64_t mergedLen = 0;
    int64_t curStart = intervals[0].start;
    int64_t curEnd = intervals[0].end;
    for (size_t k = 1; k < intervals.size(); ++k) {
        if (intervals[k].start <= curEnd) {
            if (intervals[k].end > curEnd) {
                curEnd = intervals[k].end;
            }
        } else {
            mergedLen += curEnd - curStart;
            curStart = intervals[k].start;
            curEnd = intervals[k].end;
        }
    }
    mergedLen += curEnd - curStart;
    return mergedLen;
}

bool CanMergeRects(const SelectionRect &ri, const SelectionRect &rj, int32_t tol)
{
    int32_t hx = SelectionGeometryUtils::HorizontalGap(ri, rj);
    int32_t vy = SelectionGeometryUtils::VerticalGap(ri, rj);
    bool xOverlap = (hx <= 0);
    bool yOverlap = (vy <= 0);
    bool xAdj = (hx >= 0 && hx <= tol);
    bool yAdj = (vy >= 0 && vy <= tol);
    return (xOverlap && (yOverlap || yAdj)) || (yOverlap && (xOverlap || xAdj));
}

size_t FindMergePartner(const std::vector<SelectionRect> &rects, size_t i, int32_t tol)
{
    for (size_t j = i + 1; j < rects.size(); ++j) {
        if (CanMergeRects(rects[i], rects[j], tol)) {
            return j;
        }
    }
    return rects.size();
}

bool IsContainedByAny(size_t idx, const std::vector<SelectionRect> &list)
{
    for (size_t j = 0; j < list.size(); ++j) {
        if (idx == j) {
            continue;
        }
        if (list[idx] == list[j]) {
            if (j < idx) {
                return true;
            }
            continue;
        }
        if (SelectionGeometryUtils::ContainsRect(list[j], list[idx])) {
            return true;
        }
    }
    return false;
}

size_t FindRowGroup(const SelectionRect &rect, const std::vector<std::vector<SelectionRect>> &groups,
    int32_t rowTol)
{
    for (size_t g = 0; g < groups.size(); ++g) {
        if (groups[g].empty()) {
            continue;
        }
        SelectionRect bounds = SelectionGeometryUtils::UnionListBounds(groups[g]);
        if (OverlapOrAdjacent1D(bounds.y, SelectionGeometryUtils::BottomEdge(bounds), rect.y,
            SelectionGeometryUtils::BottomEdge(rect), rowTol)) {
            return g;
        }
    }
    return groups.size();
}
} // namespace

bool SelectionRect::IsEmpty() const
{
    return width <= 0 || height <= 0;
}

bool SelectionRect::IsNormalized() const
{
    return width >= 0 && height >= 0;
}

int64_t SelectionRect::Area() const
{
    int64_t w = AbsInt64(static_cast<int64_t>(width));
    int64_t h = AbsInt64(static_cast<int64_t>(height));
    return w * h;
}

bool SelectionRect::operator==(const SelectionRect &other) const
{
    return x == other.x && y == other.y && width == other.width && height == other.height;
}

bool SelectionRect::operator!=(const SelectionRect &other) const
{
    return !(*this == other);
}

int32_t SelectionGeometryUtils::ClampInt(int32_t value, int32_t lo, int32_t hi)
{
    if (lo > hi) {
        std::swap(lo, hi);
    }
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

int32_t SelectionGeometryUtils::ClampToInt32(int64_t value)
{
    if (value < INT32_MIN_VALUE) {
        return INT32_MIN_VALUE;
    }
    if (value > INT32_MAX_VALUE) {
        return INT32_MAX_VALUE;
    }
    return static_cast<int32_t>(value);
}

int32_t SelectionGeometryUtils::SnapToPixel(double value)
{
    if (value <= static_cast<double>(INT32_MIN_VALUE)) {
        return INT32_MIN_VALUE;
    }
    if (value >= static_cast<double>(INT32_MAX_VALUE)) {
        return INT32_MAX_VALUE;
    }
    if (value >= 0.0) {
        return static_cast<int32_t>(std::floor(value + ROUND_HALF));
    }
    return static_cast<int32_t>(std::ceil(value - ROUND_HALF));
}

int32_t SelectionGeometryUtils::SafeAddInt32(int32_t a, int32_t b)
{
    int64_t sum = static_cast<int64_t>(a) + static_cast<int64_t>(b);
    return ClampToInt32(sum);
}

int32_t SelectionGeometryUtils::SafeSubInt32(int32_t a, int32_t b)
{
    int64_t diff = static_cast<int64_t>(a) - static_cast<int64_t>(b);
    return ClampToInt32(diff);
}

int64_t SelectionGeometryUtils::SafeMulInt64(int32_t a, int32_t b)
{
    return static_cast<int64_t>(a) * static_cast<int64_t>(b);
}

SelectionRect SelectionGeometryUtils::MakeEmptyRect()
{
    return SelectionRect(0, 0, 0, 0);
}

SelectionRect SelectionGeometryUtils::NormalizeRect(const SelectionRect &rect)
{
    return ClampRect(static_cast<int64_t>(rect.x), static_cast<int64_t>(rect.y),
        static_cast<int64_t>(rect.width), static_cast<int64_t>(rect.height));
}

int64_t SelectionGeometryUtils::RightEdge(const SelectionRect &rect)
{
    return static_cast<int64_t>(rect.x) + static_cast<int64_t>(rect.width);
}

int64_t SelectionGeometryUtils::BottomEdge(const SelectionRect &rect)
{
    return static_cast<int64_t>(rect.y) + static_cast<int64_t>(rect.height);
}

int64_t SelectionGeometryUtils::CenterX(const SelectionRect &rect)
{
    int64_t w = AbsInt64(static_cast<int64_t>(rect.width));
    return static_cast<int64_t>(rect.x) + w / HALF_FACTOR;
}

int64_t SelectionGeometryUtils::CenterY(const SelectionRect &rect)
{
    int64_t h = AbsInt64(static_cast<int64_t>(rect.height));
    return static_cast<int64_t>(rect.y) + h / HALF_FACTOR;
}

int64_t SelectionGeometryUtils::Perimeter(const SelectionRect &rect)
{
    int64_t w = AbsInt64(static_cast<int64_t>(rect.width));
    int64_t h = AbsInt64(static_cast<int64_t>(rect.height));
    return PERIMETER_FACTOR * (w + h);
}

double SelectionGeometryUtils::AspectRatio(const SelectionRect &rect)
{
    int64_t w = AbsInt64(static_cast<int64_t>(rect.width));
    int64_t h = AbsInt64(static_cast<int64_t>(rect.height));
    if (h == 0) {
        return 0.0;
    }
    return static_cast<double>(w) / static_cast<double>(h);
}

bool SelectionGeometryUtils::ContainsPoint(const SelectionRect &rect, const SelectionPoint &point)
{
    return ContainsPoint(rect, point.x, point.y);
}

bool SelectionGeometryUtils::ContainsPoint(const SelectionRect &rect, int32_t px, int32_t py)
{
    if (rect.IsEmpty()) {
        return false;
    }
    int64_t x = static_cast<int64_t>(px);
    int64_t y = static_cast<int64_t>(py);
    int64_t rx = static_cast<int64_t>(rect.x);
    int64_t ry = static_cast<int64_t>(rect.y);
    int64_t r = RightEdge(rect);
    int64_t b = BottomEdge(rect);
    if (r < rx) {
        std::swap(rx, r);
    }
    if (b < ry) {
        std::swap(ry, b);
    }
    return x >= rx && x < r && y >= ry && y < b;
}

bool SelectionGeometryUtils::ContainsRect(const SelectionRect &outer, const SelectionRect &inner)
{
    if (outer.IsEmpty() || inner.IsEmpty()) {
        return false;
    }
    SelectionRect normOuter = NormalizeRect(outer);
    SelectionRect normInner = NormalizeRect(inner);
    int64_t orx = normOuter.x;
    int64_t ory = normOuter.y;
    int64_t orr = RightEdge(normOuter);
    int64_t orb = BottomEdge(normOuter);
    int64_t irx = normInner.x;
    int64_t iry = normInner.y;
    int64_t irr = RightEdge(normInner);
    int64_t irb = BottomEdge(normInner);
    return irx >= orx && iry >= ory && irr <= orr && irb <= orb;
}

bool SelectionGeometryUtils::Intersects(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t al = na.x;
    int64_t at = na.y;
    int64_t ar = RightEdge(na);
    int64_t ab = BottomEdge(na);
    int64_t bl = nb.x;
    int64_t bt = nb.y;
    int64_t br = RightEdge(nb);
    int64_t bb = BottomEdge(nb);
    return al < br && bl < ar && at < bb && bt < ab;
}

bool SelectionGeometryUtils::EqualsTolerance(const SelectionRect &a, const SelectionRect &b, int32_t tol)
{
    int32_t absTol = tol < 0 ? -tol : tol;
    int32_t dx = SafeSubInt32(a.x, b.x);
    if (dx < 0) {
        dx = -dx;
    }
    int32_t dy = SafeSubInt32(a.y, b.y);
    if (dy < 0) {
        dy = -dy;
    }
    int32_t dw = SafeSubInt32(a.width, b.width);
    if (dw < 0) {
        dw = -dw;
    }
    int32_t dh = SafeSubInt32(a.height, b.height);
    if (dh < 0) {
        dh = -dh;
    }
    return dx <= absTol && dy <= absTol && dw <= absTol && dh <= absTol;
}

SelectionRect SelectionGeometryUtils::IntersectRect(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return MakeEmptyRect();
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t l = std::max(static_cast<int64_t>(na.x), static_cast<int64_t>(nb.x));
    int64_t t = std::max(static_cast<int64_t>(na.y), static_cast<int64_t>(nb.y));
    int64_t r = std::min(RightEdge(na), RightEdge(nb));
    int64_t btm = std::min(BottomEdge(na), BottomEdge(nb));
    if (r <= l || btm <= t) {
        return MakeEmptyRect();
    }
    return ClampRect(l, t, r - l, btm - t);
}

SelectionRect SelectionGeometryUtils::UnionRect(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty()) {
        return NormalizeRect(b);
    }
    if (b.IsEmpty()) {
        return NormalizeRect(a);
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t l = std::min(static_cast<int64_t>(na.x), static_cast<int64_t>(nb.x));
    int64_t t = std::min(static_cast<int64_t>(na.y), static_cast<int64_t>(nb.y));
    int64_t r = std::max(RightEdge(na), RightEdge(nb));
    int64_t btm = std::max(BottomEdge(na), BottomEdge(nb));
    return ClampRect(l, t, r - l, btm - t);
}

std::vector<SelectionRect> SelectionGeometryUtils::SubtractRect(const SelectionRect &a, const SelectionRect &b)
{
    std::vector<SelectionRect> result;
    if (a.IsEmpty()) {
        return result;
    }
    SelectionRect na = NormalizeRect(a);
    if (b.IsEmpty()) {
        result.push_back(na);
        return result;
    }
    SelectionRect inter = IntersectRect(na, b);
    if (inter.IsEmpty()) {
        result.push_back(na);
        return result;
    }
    int64_t al = na.x;
    int64_t at = na.y;
    int64_t ar = RightEdge(na);
    int64_t ab = BottomEdge(na);
    int64_t il = inter.x;
    int64_t it = inter.y;
    int64_t ir = RightEdge(inter);
    int64_t ib = BottomEdge(inter);
    if (il > al) {
        result.push_back(ClampRect(al, at, il - al, ab - at));
    }
    if (ir < ar) {
        result.push_back(ClampRect(ir, at, ar - ir, ab - at));
    }
    if (it > at) {
        result.push_back(ClampRect(il, at, ir - il, it - at));
    }
    if (ib < ab) {
        result.push_back(ClampRect(il, ib, ir - il, ab - ib));
    }
    return result;
}

std::vector<SelectionRect> SelectionGeometryUtils::IntersectListWithRect(const std::vector<SelectionRect> &list,
    const SelectionRect &rect)
{
    std::vector<SelectionRect> result;
    for (const auto &item : list) {
        SelectionRect inter = IntersectRect(item, rect);
        if (!inter.IsEmpty()) {
            result.push_back(inter);
        }
    }
    return result;
}

SelectionRect SelectionGeometryUtils::UnionListBounds(const std::vector<SelectionRect> &list)
{
    if (list.empty()) {
        return MakeEmptyRect();
    }
    SelectionRect acc = NormalizeRect(list.front());
    for (size_t i = 1; i < list.size(); ++i) {
        acc = UnionRect(acc, list[i]);
    }
    return acc;
}

double SelectionGeometryUtils::DistancePointRect(const SelectionPoint &point, const SelectionRect &rect)
{
    if (rect.IsEmpty()) {
        return 0.0;
    }
    SelectionRect nr = NormalizeRect(rect);
    int64_t px = point.x;
    int64_t py = point.y;
    int64_t l = nr.x;
    int64_t t = nr.y;
    int64_t r = RightEdge(nr);
    int64_t b = BottomEdge(nr);
    int64_t dx = 0;
    int64_t dy = 0;
    if (px < l) {
        dx = l - px;
    } else if (px >= r) {
        dx = px - r + 1;
    }
    if (py < t) {
        dy = t - py;
    } else if (py >= b) {
        dy = py - b + 1;
    }
    double ddx = static_cast<double>(dx);
    double ddy = static_cast<double>(dy);
    return std::sqrt(ddx * ddx + ddy * ddy);
}

int32_t SelectionGeometryUtils::SignedDistanceXPointRect(const SelectionPoint &point, const SelectionRect &rect)
{
    if (rect.IsEmpty()) {
        return 0;
    }
    SelectionRect nr = NormalizeRect(rect);
    int64_t px = point.x;
    int64_t l = nr.x;
    int64_t r = RightEdge(nr);
    if (px < l) {
        return ClampToInt32(px - l);
    }
    if (px >= r) {
        return ClampToInt32(px - r + 1);
    }
    return 0;
}

int32_t SelectionGeometryUtils::SignedDistanceYPointRect(const SelectionPoint &point, const SelectionRect &rect)
{
    if (rect.IsEmpty()) {
        return 0;
    }
    SelectionRect nr = NormalizeRect(rect);
    int64_t py = point.y;
    int64_t t = nr.y;
    int64_t b = BottomEdge(nr);
    if (py < t) {
        return ClampToInt32(py - t);
    }
    if (py >= b) {
        return ClampToInt32(py - b + 1);
    }
    return 0;
}

double SelectionGeometryUtils::DistanceRectRect(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return 0.0;
    }
    int32_t hx = HorizontalGap(a, b);
    int32_t vy = VerticalGap(a, b);
    if (hx <= 0 && vy <= 0) {
        return 0.0;
    }
    double dx = static_cast<double>(hx > 0 ? hx : 0);
    double dy = static_cast<double>(vy > 0 ? vy : 0);
    return std::sqrt(dx * dx + dy * dy);
}

int32_t SelectionGeometryUtils::HorizontalGap(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return 0;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t al = na.x;
    int64_t ar = RightEdge(na);
    int64_t bl = nb.x;
    int64_t br = RightEdge(nb);
    if (ar <= bl) {
        return ClampToInt32(bl - ar);
    }
    if (br <= al) {
        return ClampToInt32(al - br);
    }
    return 0;
}

int32_t SelectionGeometryUtils::VerticalGap(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return 0;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t at = na.y;
    int64_t ab = BottomEdge(na);
    int64_t bt = nb.y;
    int64_t bb = BottomEdge(nb);
    if (ab <= bt) {
        return ClampToInt32(bt - ab);
    }
    if (bb <= at) {
        return ClampToInt32(at - bb);
    }
    return 0;
}

bool SelectionGeometryUtils::IsAdjacent(const SelectionRect &a, const SelectionRect &b, int32_t tol)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int32_t hx = HorizontalGap(na, nb);
    int32_t vy = VerticalGap(na, nb);
    int32_t absTol = tol < 0 ? -tol : tol;
    bool xAdj = (hx >= 0 && hx <= absTol) && Overlap1D(na.y, BottomEdge(na), nb.y, BottomEdge(nb));
    bool yAdj = (vy >= 0 && vy <= absTol) && Overlap1D(na.x, RightEdge(na), nb.x, RightEdge(nb));
    return xAdj || yAdj;
}

bool SelectionGeometryUtils::IsHorizontallyAligned(const SelectionRect &a, const SelectionRect &b, int32_t tol)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t at = na.y;
    int64_t ab = BottomEdge(na);
    int64_t bt = nb.y;
    int64_t bb = BottomEdge(nb);
    int64_t ac = (at + ab) / HALF_FACTOR;
    int64_t bc = (bt + bb) / HALF_FACTOR;
    int64_t diff = ac - bc;
    if (diff < 0) {
        diff = -diff;
    }
    int64_t absTol = tol < 0 ? -static_cast<int64_t>(tol) : static_cast<int64_t>(tol);
    return diff <= absTol;
}

bool SelectionGeometryUtils::IsVerticallyAligned(const SelectionRect &a, const SelectionRect &b, int32_t tol)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    int64_t al = na.x;
    int64_t ar = RightEdge(na);
    int64_t bl = nb.x;
    int64_t br = RightEdge(nb);
    int64_t ac = (al + ar) / HALF_FACTOR;
    int64_t bc = (bl + br) / HALF_FACTOR;
    int64_t diff = ac - bc;
    if (diff < 0) {
        diff = -diff;
    }
    int64_t absTol = tol < 0 ? -static_cast<int64_t>(tol) : static_cast<int64_t>(tol);
    return diff <= absTol;
}

bool SelectionGeometryUtils::AreOnSameRow(const SelectionRect &a, const SelectionRect &b, int32_t tol)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    return OverlapOrAdjacent1D(na.y, BottomEdge(na), nb.y, BottomEdge(nb), tol);
}

bool SelectionGeometryUtils::AreOnSameColumn(const SelectionRect &a, const SelectionRect &b, int32_t tol)
{
    if (a.IsEmpty() || b.IsEmpty()) {
        return false;
    }
    SelectionRect na = NormalizeRect(a);
    SelectionRect nb = NormalizeRect(b);
    return OverlapOrAdjacent1D(na.x, RightEdge(na), nb.x, RightEdge(nb), tol);
}

SelectionRect SelectionGeometryUtils::Inflate(const SelectionRect &rect, int32_t left, int32_t top,
    int32_t right, int32_t bottom)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t x = static_cast<int64_t>(nr.x) - static_cast<int64_t>(left);
    int64_t y = static_cast<int64_t>(nr.y) - static_cast<int64_t>(top);
    int64_t w = static_cast<int64_t>(nr.width) + static_cast<int64_t>(left) + static_cast<int64_t>(right);
    int64_t h = static_cast<int64_t>(nr.height) + static_cast<int64_t>(top) + static_cast<int64_t>(bottom);
    return ClampRect(x, y, w, h);
}

SelectionRect SelectionGeometryUtils::InflateUniform(const SelectionRect &rect, int32_t amount)
{
    return Inflate(rect, amount, amount, amount, amount);
}

SelectionRect SelectionGeometryUtils::Deflate(const SelectionRect &rect, int32_t left, int32_t top,
    int32_t right, int32_t bottom)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t x = static_cast<int64_t>(nr.x) + static_cast<int64_t>(left);
    int64_t y = static_cast<int64_t>(nr.y) + static_cast<int64_t>(top);
    int64_t w = static_cast<int64_t>(nr.width) - static_cast<int64_t>(left) - static_cast<int64_t>(right);
    int64_t h = static_cast<int64_t>(nr.height) - static_cast<int64_t>(top) - static_cast<int64_t>(bottom);
    return ClampRect(x, y, w, h);
}

SelectionRect SelectionGeometryUtils::DeflateUniform(const SelectionRect &rect, int32_t amount)
{
    return Deflate(rect, amount, amount, amount, amount);
}

SelectionRect SelectionGeometryUtils::Offset(const SelectionRect &rect, int32_t dx, int32_t dy)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t x = static_cast<int64_t>(nr.x) + static_cast<int64_t>(dx);
    int64_t y = static_cast<int64_t>(nr.y) + static_cast<int64_t>(dy);
    return ClampRect(x, y, nr.width, nr.height);
}

SelectionRect SelectionGeometryUtils::Scale(const SelectionRect &rect, double sx, double sy)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    double x = static_cast<double>(nr.x) * sx;
    double y = static_cast<double>(nr.y) * sy;
    double w = static_cast<double>(nr.width) * sx;
    double h = static_cast<double>(nr.height) * sy;
    return ClampRect(static_cast<int64_t>(std::floor(x)), static_cast<int64_t>(std::floor(y)),
        static_cast<int64_t>(std::floor(w)), static_cast<int64_t>(std::floor(h)));
}

SelectionRect SelectionGeometryUtils::Transpose(const SelectionRect &rect)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    return SelectionRect(nr.x, nr.y, nr.height, nr.width);
}

SelectionRect SelectionGeometryUtils::MirrorHorizontal(const SelectionRect &rect, int32_t axisX)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t axis = static_cast<int64_t>(axisX);
    int64_t l = static_cast<int64_t>(nr.x);
    int64_t r = RightEdge(nr);
    int64_t newR = MIRROR_FACTOR * axis - l;
    int64_t newL = MIRROR_FACTOR * axis - r;
    if (newL > newR) {
        std::swap(newL, newR);
    }
    return ClampRect(newL, nr.y, newR - newL, nr.height);
}

SelectionRect SelectionGeometryUtils::MirrorVertical(const SelectionRect &rect, int32_t axisY)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t axis = static_cast<int64_t>(axisY);
    int64_t t = static_cast<int64_t>(nr.y);
    int64_t b = BottomEdge(nr);
    int64_t newB = MIRROR_FACTOR * axis - t;
    int64_t newT = MIRROR_FACTOR * axis - b;
    if (newT > newB) {
        std::swap(newT, newB);
    }
    return ClampRect(nr.x, newT, nr.width, newB - newT);
}

SelectionRect SelectionGeometryUtils::Rotate90(const SelectionRect &rect, const SelectionPoint &pivot)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t px = pivot.x;
    int64_t py = pivot.y;
    int64_t l = nr.x;
    int64_t t = nr.y;
    int64_t r = RightEdge(nr);
    int64_t b = BottomEdge(nr);
    int64_t nx = px - (b - py);
    int64_t ny = py + (l - px);
    int64_t nw = nr.height;
    int64_t nh = nr.width;
    int64_t nx2 = px - (t - py);
    int64_t ny2 = py + (r - px);
    int64_t minX = std::min(nx, nx2 - nw);
    int64_t minY = std::min(ny, ny2 - nh);
    return ClampRect(minX, minY, nw, nh);
}

std::vector<SelectionRect> SelectionGeometryUtils::MergeOverlapping(const std::vector<SelectionRect> &list,
    int32_t tol)
{
    std::vector<SelectionRect> work;
    work.reserve(list.size());
    for (const auto &item : list) {
        if (!item.IsEmpty()) {
            work.push_back(NormalizeRect(item));
        }
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < work.size(); ++i) {
            size_t j = FindMergePartner(work, i, tol);
            if (j >= work.size()) {
                continue;
            }
            work[i] = UnionRect(work[i], work[j]);
            work.erase(work.begin() + static_cast<ptrdiff_t>(j));
            changed = true;
            break;
        }
    }
    return work;
}

int64_t SelectionGeometryUtils::TotalArea(const std::vector<SelectionRect> &list)
{
    int64_t total = 0;
    for (const auto &item : list) {
        SelectionRect nr = NormalizeRect(item);
        total += static_cast<int64_t>(AbsInt64(nr.width)) * static_cast<int64_t>(AbsInt64(nr.height));
    }
    return total;
}

SelectionRect SelectionGeometryUtils::BoundsOfList(const std::vector<SelectionRect> &list)
{
    return UnionListBounds(list);
}

bool SelectionGeometryUtils::ContainsAny(const std::vector<SelectionRect> &list, const SelectionRect &rect)
{
    for (const auto &item : list) {
        if (ContainsRect(item, rect)) {
            return true;
        }
    }
    return false;
}

bool SelectionGeometryUtils::ContainsAll(const std::vector<SelectionRect> &list, const SelectionRect &rect)
{
    if (list.empty()) {
        return false;
    }
    for (const auto &item : list) {
        if (!ContainsRect(item, rect)) {
            return false;
        }
    }
    return true;
}

std::vector<SelectionRect> SelectionGeometryUtils::RemoveContained(const std::vector<SelectionRect> &list)
{
    std::vector<SelectionRect> normed;
    normed.reserve(list.size());
    for (const auto &item : list) {
        if (!item.IsEmpty()) {
            normed.push_back(NormalizeRect(item));
        }
    }
    std::vector<SelectionRect> result;
    result.reserve(normed.size());
    for (size_t i = 0; i < normed.size(); ++i) {
        if (!IsContainedByAny(i, normed)) {
            result.push_back(normed[i]);
        }
    }
    return result;
}

std::vector<SelectionRect> SelectionGeometryUtils::Deduplicate(const std::vector<SelectionRect> &list,
    int32_t tol)
{
    std::vector<SelectionRect> normed;
    normed.reserve(list.size());
    for (const auto &item : list) {
        if (!item.IsEmpty()) {
            normed.push_back(NormalizeRect(item));
        }
    }
    std::vector<SelectionRect> result;
    result.reserve(normed.size());
    for (const auto &item : normed) {
        bool dup = false;
        for (const auto &kept : result) {
            if (EqualsTolerance(item, kept, tol)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            result.push_back(item);
        }
    }
    return result;
}

std::vector<std::vector<SelectionRect>> SelectionGeometryUtils::GroupByRows(
    const std::vector<SelectionRect> &list, int32_t rowTol)
{
    std::vector<SelectionRect> sorted = SortByRow(list);
    std::vector<std::vector<SelectionRect>> groups;
    for (const auto &rect : sorted) {
        if (rect.IsEmpty()) {
            continue;
        }
        size_t idx = FindRowGroup(rect, groups, rowTol);
        if (idx >= groups.size()) {
            groups.push_back(std::vector<SelectionRect>{rect});
        } else {
            groups[idx].push_back(rect);
        }
    }
    return groups;
}

std::vector<SelectionRect> SelectionGeometryUtils::SortByRow(const std::vector<SelectionRect> &list)
{
    std::vector<SelectionRect> normed;
    normed.reserve(list.size());
    for (const auto &item : list) {
        if (!item.IsEmpty()) {
            normed.push_back(NormalizeRect(item));
        }
    }
    std::sort(normed.begin(), normed.end(), [](const SelectionRect &a, const SelectionRect &b) {
        if (a.y != b.y) {
            return a.y < b.y;
        }
        if (a.x != b.x) {
            return a.x < b.x;
        }
        if (a.height != b.height) {
            return a.height < b.height;
        }
        return a.width < b.width;
    });
    return normed;
}

SelectionRect SelectionGeometryUtils::ClipToBounds(const SelectionRect &rect, const SelectionRect &bounds)
{
    return IntersectRect(rect, bounds);
}

std::vector<SelectionRect> SelectionGeometryUtils::ClipRectListToBounds(const std::vector<SelectionRect> &list,
    const SelectionRect &bounds)
{
    return IntersectListWithRect(list, bounds);
}

bool SelectionGeometryUtils::ClipSegmentToRect(SelectionPoint &p1, SelectionPoint &p2,
    const SelectionRect &rect)
{
    if (rect.IsEmpty()) {
        return false;
    }
    SelectionRect nr = NormalizeRect(rect);
    double x1 = static_cast<double>(p1.x);
    double y1 = static_cast<double>(p1.y);
    double x2 = static_cast<double>(p2.x);
    double y2 = static_cast<double>(p2.y);
    double dx = x2 - x1;
    double dy = y2 - y1;
    double p[RECT_EDGE_COUNT] = { -dx, dx, -dy, dy };
    double q[RECT_EDGE_COUNT] = { x1 - static_cast<double>(nr.x),
        static_cast<double>(RightEdge(nr)) - x1,
        y1 - static_cast<double>(nr.y),
        static_cast<double>(BottomEdge(nr)) - y1 };
    double t0 = CLIP_T_MIN;
    double t1 = CLIP_T_MAX;
    for (int i = 0; i < RECT_EDGE_COUNT; ++i) {
        if (!ApplySegmentClipEdge(p[i], q[i], t0, t1)) {
            return false;
        }
    }
    double nx1 = x1 + t0 * dx;
    double ny1 = y1 + t0 * dy;
    double nx2 = x1 + t1 * dx;
    double ny2 = y1 + t1 * dy;
    p1.x = ClampToInt32(static_cast<int64_t>(std::round(nx1)));
    p1.y = ClampToInt32(static_cast<int64_t>(std::round(ny1)));
    p2.x = ClampToInt32(static_cast<int64_t>(std::round(nx2)));
    p2.y = ClampToInt32(static_cast<int64_t>(std::round(ny2)));
    return true;
}

bool SelectionGeometryUtils::SegmentIntersectsRect(const SelectionPoint &p1, const SelectionPoint &p2,
    const SelectionRect &rect)
{
    if (rect.IsEmpty()) {
        return false;
    }
    SelectionRect nr = NormalizeRect(rect);
    SelectionPoint s1 = p1;
    SelectionPoint s2 = p2;
    if (ClipSegmentToRect(s1, s2, nr)) {
        return true;
    }
    return ContainsPoint(nr, p1) || ContainsPoint(nr, p2);
}

std::vector<SelectionPoint> SelectionGeometryUtils::ClipPolygonToRect(
    const std::vector<SelectionPoint> &polygon, const SelectionRect &rect)
{
    if (polygon.size() < MIN_POLYGON_VERTICES || rect.IsEmpty()) {
        return std::vector<SelectionPoint>();
    }
    SelectionRect nr = NormalizeRect(rect);
    std::vector<SelectionPoint> output = polygon;
    for (int edge = 0; edge < RECT_EDGE_COUNT; ++edge) {
        output = ClipPolygonSingleEdge(output, edge, nr);
        if (output.empty()) {
            return output;
        }
    }
    return output;
}

int64_t SelectionGeometryUtils::UnionArea(const std::vector<SelectionRect> &list)
{
    if (list.empty()) {
        return 0;
    }
    std::vector<SelectionRect> normed;
    normed.reserve(list.size());
    for (const auto &item : list) {
        if (!item.IsEmpty()) {
            normed.push_back(NormalizeRect(item));
        }
    }
    if (normed.empty()) {
        return 0;
    }
    std::vector<int64_t> ys;
    ys.reserve(normed.size() * Y_BOUNDS_PER_RECT);
    for (const auto &rect : normed) {
        ys.push_back(rect.y);
        ys.push_back(BottomEdge(rect));
    }
    std::sort(ys.begin(), ys.end());
    ys.erase(std::unique(ys.begin(), ys.end()), ys.end());
    int64_t total = 0;
    for (size_t i = 0; i + 1 < ys.size(); ++i) {
        int64_t y0 = ys[i];
        int64_t y1 = ys[i + 1];
        if (y1 <= y0) {
            continue;
        }
        std::vector<Interval> intervals;
        for (const auto &rect : normed) {
            if (rect.y <= y0 && BottomEdge(rect) >= y1) {
                intervals.push_back({ rect.x, RightEdge(rect) });
            }
        }
        if (intervals.empty()) {
            continue;
        }
        total += MergeIntervalLengths(intervals) * (y1 - y0);
    }
    return total;
}

SelectionRect SelectionGeometryUtils::MapFromParent(const SelectionRect &rect,
    const SelectionPoint &parentOrigin)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t x = static_cast<int64_t>(nr.x) - static_cast<int64_t>(parentOrigin.x);
    int64_t y = static_cast<int64_t>(nr.y) - static_cast<int64_t>(parentOrigin.y);
    return ClampRect(x, y, nr.width, nr.height);
}

SelectionRect SelectionGeometryUtils::MapToParent(const SelectionRect &rect,
    const SelectionPoint &parentOrigin)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t x = static_cast<int64_t>(nr.x) + static_cast<int64_t>(parentOrigin.x);
    int64_t y = static_cast<int64_t>(nr.y) + static_cast<int64_t>(parentOrigin.y);
    return ClampRect(x, y, nr.width, nr.height);
}

SelectionRect SelectionGeometryUtils::NormalizeForRTL(const SelectionRect &rect, int32_t parentWidth)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty() || parentWidth <= 0) {
        return nr;
    }
    int64_t pw = static_cast<int64_t>(parentWidth);
    int64_t l = nr.x;
    int64_t r = RightEdge(nr);
    int64_t newR = pw - l;
    int64_t newL = pw - r;
    if (newL > newR) {
        std::swap(newL, newR);
    }
    return ClampRect(newL, nr.y, newR - newL, nr.height);
}

SelectionRect SelectionGeometryUtils::MinRectByArea(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty()) {
        return b;
    }
    if (b.IsEmpty()) {
        return a;
    }
    return a.Area() <= b.Area() ? a : b;
}

SelectionRect SelectionGeometryUtils::MaxRectByArea(const SelectionRect &a, const SelectionRect &b)
{
    if (a.IsEmpty()) {
        return b;
    }
    if (b.IsEmpty()) {
        return a;
    }
    return a.Area() >= b.Area() ? a : b;
}

double SelectionGeometryUtils::PointDistance(const SelectionPoint &a, const SelectionPoint &b)
{
    double dx = static_cast<double>(a.x - b.x);
    double dy = static_cast<double>(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

bool SelectionGeometryUtils::PointEqualsTolerance(const SelectionPoint &a, const SelectionPoint &b,
    int32_t tol)
{
    int32_t absTol = tol < 0 ? -tol : tol;
    int32_t dx = SafeSubInt32(a.x, b.x);
    if (dx < 0) {
        dx = -dx;
    }
    int32_t dy = SafeSubInt32(a.y, b.y);
    if (dy < 0) {
        dy = -dy;
    }
    return dx <= absTol && dy <= absTol;
}

SelectionPoint SelectionGeometryUtils::PointLerp(const SelectionPoint &a, const SelectionPoint &b,
    double t)
{
    if (t < CLIP_T_MIN) {
        t = CLIP_T_MIN;
    } else if (t > CLIP_T_MAX) {
        t = CLIP_T_MAX;
    }
    double x = static_cast<double>(a.x) + t * static_cast<double>(b.x - a.x);
    double y = static_cast<double>(a.y) + t * static_cast<double>(b.y - a.y);
    return SelectionPoint(SnapToPixel(x), SnapToPixel(y));
}

SelectionPoint SelectionGeometryUtils::PointTranslate(const SelectionPoint &pt, int32_t dx, int32_t dy)
{
    int64_t x = static_cast<int64_t>(pt.x) + static_cast<int64_t>(dx);
    int64_t y = static_cast<int64_t>(pt.y) + static_cast<int64_t>(dy);
    return SelectionPoint(ClampToInt32(x), ClampToInt32(y));
}

SelectionPoint SelectionGeometryUtils::PointRotate90(const SelectionPoint &pt, const SelectionPoint &pivot)
{
    int64_t dx = static_cast<int64_t>(pt.x) - static_cast<int64_t>(pivot.x);
    int64_t dy = static_cast<int64_t>(pt.y) - static_cast<int64_t>(pivot.y);
    int64_t nx = static_cast<int64_t>(pivot.x) - dy;
    int64_t ny = static_cast<int64_t>(pivot.y) + dx;
    return SelectionPoint(ClampToInt32(nx), ClampToInt32(ny));
}

SelectionPoint SelectionGeometryUtils::PointMirrorHorizontal(const SelectionPoint &pt, int32_t axisX)
{
    int64_t axis = static_cast<int64_t>(axisX);
    int64_t x = static_cast<int64_t>(pt.x);
    int64_t nx = MIRROR_FACTOR * axis - x;
    return SelectionPoint(ClampToInt32(nx), pt.y);
}

SelectionPoint SelectionGeometryUtils::PointMirrorVertical(const SelectionPoint &pt, int32_t axisY)
{
    int64_t axis = static_cast<int64_t>(axisY);
    int64_t y = static_cast<int64_t>(pt.y);
    int64_t ny = MIRROR_FACTOR * axis - y;
    return SelectionPoint(pt.x, ClampToInt32(ny));
}

double SelectionGeometryUtils::SegmentLength(const SelectionPoint &a, const SelectionPoint &b)
{
    return PointDistance(a, b);
}

double SelectionGeometryUtils::PointToSegmentDistance(const SelectionPoint &p, const SelectionPoint &s,
    const SelectionPoint &e)
{
    int64_t px = p.x;
    int64_t py = p.y;
    int64_t sx = s.x;
    int64_t sy = s.y;
    int64_t ex = e.x;
    int64_t ey = e.y;
    int64_t dx = ex - sx;
    int64_t dy = ey - sy;
    int64_t lenSq = dx * dx + dy * dy;
    if (lenSq == 0) {
        double ddx = static_cast<double>(px - sx);
        double ddy = static_cast<double>(py - sy);
        return std::sqrt(ddx * ddx + ddy * ddy);
    }
    int64_t crossX = px - sx;
    int64_t crossY = py - sy;
    double t = static_cast<double>(crossX * dx + crossY * dy) / static_cast<double>(lenSq);
    if (t < CLIP_T_MIN) {
        t = CLIP_T_MIN;
    } else if (t > CLIP_T_MAX) {
        t = CLIP_T_MAX;
    }
    double projX = static_cast<double>(sx) + t * static_cast<double>(dx);
    double projY = static_cast<double>(sy) + t * static_cast<double>(dy);
    double ddx = static_cast<double>(px) - projX;
    double ddy = static_cast<double>(py) - projY;
    return std::sqrt(ddx * ddx + ddy * ddy);
}

SelectionPoint SelectionGeometryUtils::FootOfPerpendicular(const SelectionPoint &p, const SelectionPoint &s,
    const SelectionPoint &e)
{
    int64_t sx = s.x;
    int64_t sy = s.y;
    int64_t ex = e.x;
    int64_t ey = e.y;
    int64_t dx = ex - sx;
    int64_t dy = ey - sy;
    int64_t lenSq = dx * dx + dy * dy;
    if (lenSq == 0) {
        return s;
    }
    int64_t crossX = static_cast<int64_t>(p.x) - sx;
    int64_t crossY = static_cast<int64_t>(p.y) - sy;
    double t = static_cast<double>(crossX * dx + crossY * dy) / static_cast<double>(lenSq);
    double projX = static_cast<double>(sx) + t * static_cast<double>(dx);
    double projY = static_cast<double>(sy) + t * static_cast<double>(dy);
    return SelectionPoint(SnapToPixel(projX), SnapToPixel(projY));
}

bool SelectionGeometryUtils::IsPointOnSegment(const SelectionPoint &p, const SelectionPoint &s,
    const SelectionPoint &e)
{
    int64_t px = p.x;
    int64_t py = p.y;
    int64_t sx = s.x;
    int64_t sy = s.y;
    int64_t ex = e.x;
    int64_t ey = e.y;
    int64_t cross = (ex - sx) * (py - sy) - (ey - sy) * (px - sx);
    if (cross != 0) {
        return false;
    }
    int64_t dot = (px - sx) * (px - ex) + (py - sy) * (py - ey);
    return dot <= 0;
}

std::vector<SelectionPoint> SelectionGeometryUtils::RectCorners(const SelectionRect &rect)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return std::vector<SelectionPoint>();
    }
    int32_t l = nr.x;
    int32_t t = nr.y;
    int32_t r = ClampToInt32(RightEdge(nr));
    int32_t b = ClampToInt32(BottomEdge(nr));
    return std::vector<SelectionPoint>{
        SelectionPoint(l, t),
        SelectionPoint(r, t),
        SelectionPoint(r, b),
        SelectionPoint(l, b),
    };
}

std::vector<SelectionPoint> SelectionGeometryUtils::RectEdgeMidpoints(const SelectionRect &rect)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return std::vector<SelectionPoint>();
    }
    int64_t cx = CenterX(nr);
    int64_t cy = CenterY(nr);
    int32_t l = nr.x;
    int32_t t = nr.y;
    int32_t r = ClampToInt32(RightEdge(nr));
    int32_t b = ClampToInt32(BottomEdge(nr));
    return std::vector<SelectionPoint>{
        SelectionPoint(static_cast<int32_t>(cx), t),
        SelectionPoint(r, static_cast<int32_t>(cy)),
        SelectionPoint(static_cast<int32_t>(cx), b),
        SelectionPoint(l, static_cast<int32_t>(cy)),
    };
}

int32_t SelectionGeometryUtils::ClosestEdgeIndex(const SelectionRect &rect, const SelectionPoint &point)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return -1;
    }
    int64_t px = point.x;
    int64_t py = point.y;
    int64_t l = nr.x;
    int64_t t = nr.y;
    int64_t r = RightEdge(nr);
    int64_t b = BottomEdge(nr);
    int64_t cx = CenterX(nr);
    int64_t cy = CenterY(nr);
    int64_t dTop = AbsInt64(py - t);
    int64_t dBottom = AbsInt64(py - b);
    int64_t dLeft = AbsInt64(px - l);
    int64_t dRight = AbsInt64(px - r);
    int64_t dists[RECT_EDGE_COUNT] = { dTop, dRight, dBottom, dLeft };
    int64_t cxToCenter = AbsInt64(px - cx);
    int64_t cyToCenter = AbsInt64(py - cy);
    int64_t extraTop = (py < t) ? cyToCenter : 0;
    int64_t extraBottom = (py >= b) ? cyToCenter : 0;
    int64_t extraLeft = (px < l) ? cxToCenter : 0;
    int64_t extraRight = (px >= r) ? cxToCenter : 0;
    int64_t extras[RECT_EDGE_COUNT] = { extraTop, extraRight, extraBottom, extraLeft };
    int32_t best = 0;
    for (int i = 1; i < RECT_EDGE_COUNT; ++i) {
        int64_t cur = dists[i] + extras[i];
        int64_t bestVal = dists[best] + extras[best];
        if (cur < bestVal) {
            best = i;
        }
    }
    return best;
}

SelectionRect SelectionGeometryUtils::ExpandToContain(const SelectionRect &rect, const SelectionPoint &point)
{
    if (rect.IsEmpty()) {
        return ClampRect(point.x, point.y, 1, 1);
    }
    SelectionRect nr = NormalizeRect(rect);
    int64_t l = std::min(static_cast<int64_t>(nr.x), static_cast<int64_t>(point.x));
    int64_t t = std::min(static_cast<int64_t>(nr.y), static_cast<int64_t>(point.y));
    int64_t r = std::max(RightEdge(nr), static_cast<int64_t>(point.x) + 1);
    int64_t b = std::max(BottomEdge(nr), static_cast<int64_t>(point.y) + 1);
    return ClampRect(l, t, r - l, b - t);
}

SelectionRect SelectionGeometryUtils::ShrinkToAspectRatio(const SelectionRect &rect, double targetRatio)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty() || targetRatio <= 0.0) {
        return MakeEmptyRect();
    }
    int64_t w = nr.width;
    int64_t h = nr.height;
    if (h == 0 || targetRatio == 0.0) {
        return MakeEmptyRect();
    }
    int64_t cx = CenterX(nr);
    int64_t cy = CenterY(nr);
    int64_t newW = w;
    int64_t newH = h;
    double curRatio = static_cast<double>(w) / static_cast<double>(h);
    if (curRatio > targetRatio) {
        newW = static_cast<int64_t>(std::floor(static_cast<double>(h) * targetRatio));
    } else if (curRatio < targetRatio) {
        newH = static_cast<int64_t>(std::floor(static_cast<double>(w) / targetRatio));
    }
    if (newW > w) {
        newW = w;
    }
    if (newH > h) {
        newH = h;
    }
    int64_t newL = cx - newW / HALF_FACTOR;
    int64_t newT = cy - newH / HALF_FACTOR;
    return ClampRect(newL, newT, newW, newH);
}

std::vector<SelectionRect> SelectionGeometryUtils::SplitGrid(const SelectionRect &rect, int32_t rows,
    int32_t cols)
{
    std::vector<SelectionRect> result;
    if (rect.IsEmpty()) {
        return result;
    }
    SelectionRect nr = NormalizeRect(rect);
    if (cols <= 0 || rows <= 0) {
        return result;
    }
    int64_t totalW = nr.width;
    int64_t totalH = nr.height;
    int64_t baseW = totalW / static_cast<int64_t>(cols);
    int64_t baseH = totalH / static_cast<int64_t>(rows);
    int64_t remW = totalW - baseW * static_cast<int64_t>(cols);
    int64_t remH = totalH - baseH * static_cast<int64_t>(rows);
    int64_t y = nr.y;
    for (int32_t r = 0; r < rows; ++r) {
        int64_t cellH = baseH + (r < remH ? 1 : 0);
        int64_t x = nr.x;
        for (int32_t c = 0; c < cols; ++c) {
            int64_t cellW = baseW + (c < remW ? 1 : 0);
            result.push_back(ClampRect(x, y, cellW, cellH));
            x += cellW;
        }
        y += cellH;
    }
    return result;
}

bool SelectionGeometryUtils::IsSquare(const SelectionRect &rect)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return false;
    }
    return nr.width == nr.height;
}

int32_t SelectionGeometryUtils::LargerAxis(const SelectionRect &rect)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return 0;
    }
    return nr.width >= nr.height ? nr.width : nr.height;
}

int32_t SelectionGeometryUtils::SmallerAxis(const SelectionRect &rect)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return 0;
    }
    return nr.width <= nr.height ? nr.width : nr.height;
}

SelectionRect SelectionGeometryUtils::CenterAt(const SelectionRect &rect, const SelectionPoint &center)
{
    SelectionRect nr = NormalizeRect(rect);
    if (nr.IsEmpty()) {
        return MakeEmptyRect();
    }
    int64_t cx = center.x;
    int64_t cy = center.y;
    int64_t newL = cx - static_cast<int64_t>(nr.width) / HALF_FACTOR;
    int64_t newT = cy - static_cast<int64_t>(nr.height) / HALF_FACTOR;
    return ClampRect(newL, newT, nr.width, nr.height);
}

void SelectionRectCombiner::Add(const SelectionRect &rect)
{
    if (rect.IsEmpty()) {
        return;
    }
    rects_.push_back(SelectionGeometryUtils::NormalizeRect(rect));
    TryMergeAdjacent();
}

std::vector<SelectionRect> SelectionRectCombiner::Result() const
{
    return rects_;
}

void SelectionRectCombiner::Clear()
{
    rects_.clear();
}

size_t SelectionRectCombiner::Size() const
{
    return rects_.size();
}

bool SelectionRectCombiner::Empty() const
{
    return rects_.empty();
}

void SelectionRectCombiner::TryMergeAdjacent()
{
    bool changed = true;
    while (changed) {
        changed = false;
        for (size_t i = 0; i < rects_.size(); ++i) {
            size_t j = FindMergePartner(rects_, i, 0);
            if (j >= rects_.size()) {
                continue;
            }
            rects_[i] = SelectionGeometryUtils::UnionRect(rects_[i], rects_[j]);
            rects_.erase(rects_.begin() + static_cast<ptrdiff_t>(j));
            changed = true;
            break;
        }
    }
}

} // namespace SelectionFwk
} // namespace OHOS
