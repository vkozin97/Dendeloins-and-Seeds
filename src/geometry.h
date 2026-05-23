#pragma once

#include <array>
#include <cstdint>

struct Coord {
    int x;
    int y;
};

class Geometry {
public:
    explicit Geometry(int n);

    int size() const { return n_; }
    int cellCount() const { return n_ * n_; }
    uint64_t fullBoardMask() const { return full_board_mask_; }

    int cellId(int x, int y) const;
    Coord coord(int cell_id) const;
    bool inside(int x, int y) const;

    uint64_t ray(int cell_id, int dir) const;

    int transformCell(int sym, int cell_id) const;
    int transformDir(int sym, int dir) const;
    int inverseSymmetry(int sym) const;

private:
    int n_;
    uint64_t full_board_mask_;
    std::array<std::array<uint64_t, 8>, 64> rays_{};
    std::array<std::array<int, 64>, 8> cell_transforms_{};
    std::array<std::array<int, 8>, 8> dir_transforms_{};
    std::array<int, 8> inverse_symmetry_{};
};
