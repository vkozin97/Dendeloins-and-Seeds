#include "geometry.h"

#include <stdexcept>

namespace {
constexpr std::array<Coord, 8> kDirs{{
    {1, 0},   // E
    {-1, 0},  // W
    {0, 1},   // N
    {0, -1},  // S
    {1, 1},   // NE
    {-1, 1},  // NW
    {1, -1},  // SE
    {-1, -1}  // SW
}};

Coord applySymCell(int n, int sym, Coord c) {
    switch (sym) {
        case 0: return {c.x, c.y};
        case 1: return {n - 1 - c.y, c.x};
        case 2: return {n - 1 - c.x, n - 1 - c.y};
        case 3: return {c.y, n - 1 - c.x};
        case 4: return {n - 1 - c.x, c.y};
        case 5: return {c.x, n - 1 - c.y};
        case 6: return {c.y, c.x};
        case 7: return {n - 1 - c.y, n - 1 - c.x};
        default: return c;
    }
}
}  // namespace

Geometry::Geometry(int n) : n_(n) {
    if (n_ <= 0 || n_ > 8) {
        throw std::invalid_argument("Geometry supports only 1..8");
    }
    const int cells = cellCount();
    full_board_mask_ = (cells == 64) ? ~0ULL : ((1ULL << cells) - 1ULL);

    for (int c = 0; c < cells; ++c) {
        const Coord start = coord(c);
        for (int d = 0; d < 8; ++d) {
            uint64_t mask = 0;
            int x = start.x + kDirs[d].x;
            int y = start.y + kDirs[d].y;
            while (inside(x, y)) {
                mask |= (1ULL << cellId(x, y));
                x += kDirs[d].x;
                y += kDirs[d].y;
            }
            rays_[c][d] = mask;
        }
    }

    for (int sym = 0; sym < 8; ++sym) {
        for (int c = 0; c < cells; ++c) {
            cell_transforms_[sym][c] = cellId(applySymCell(n_, sym, coord(c)).x, applySymCell(n_, sym, coord(c)).y);
        }
        for (int d = 0; d < 8; ++d) {
            Coord v=kDirs[d];
            Coord transformed_vec{};
            switch (sym) {
                case 0: transformed_vec={v.x,v.y}; break;
                case 1: transformed_vec={-v.y,v.x}; break;
                case 2: transformed_vec={-v.x,-v.y}; break;
                case 3: transformed_vec={v.y,-v.x}; break;
                case 4: transformed_vec={-v.x,v.y}; break;
                case 5: transformed_vec={v.x,-v.y}; break;
                case 6: transformed_vec={v.y,v.x}; break;
                case 7: transformed_vec={-v.y,-v.x}; break;
            }
            int mapped = 0;
            for (; mapped < 8; ++mapped) {
                if (kDirs[mapped].x == transformed_vec.x && kDirs[mapped].y == transformed_vec.y) {
                    break;
                }
            }
            dir_transforms_[sym][d] = mapped;
        }
    }

    for (int s = 0; s < 8; ++s) {
        for (int t = 0; t < 8; ++t) {
            Coord c{1, 2};
            Coord a = applySymCell(n_, s, c);
            Coord b = applySymCell(n_, t, a);
            if (b.x == c.x && b.y == c.y) {
                inverse_symmetry_[s] = t;
                break;
            }
        }
    }
}

int Geometry::cellId(int x, int y) const { return y * n_ + x; }

Coord Geometry::coord(int cell_id) const { return {cell_id % n_, cell_id / n_}; }

bool Geometry::inside(int x, int y) const { return x >= 0 && x < n_ && y >= 0 && y < n_; }

uint64_t Geometry::ray(int cell_id, int dir) const { return rays_[cell_id][dir]; }

int Geometry::transformCell(int sym, int cell_id) const { return cell_transforms_[sym][cell_id]; }

int Geometry::transformDir(int sym, int dir) const { return dir_transforms_[sym][dir]; }

int Geometry::inverseSymmetry(int sym) const { return inverse_symmetry_[sym]; }
