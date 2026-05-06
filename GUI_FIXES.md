# GUI Fixes - Ice Sliding Puzzle Solver

## Perbaikan yang Dilakukan (6 Mei 2026)

### 1. **Hitbox Alignment Fix (Button Click Detection)**
**Masalah**: Button dan dropdown tidak merespons di tempat klik yang diharapkan - perlu klik agak di atas.

**Penyebab**: Dropdown option hitbox calculation tidak selaras dengan rendering position.

**Solusi**:
- Tambah field `int expandDirection` ke struct `Dropdown` untuk track direction expansion (1=down, -1=up)
- Update `optionBounds()` untuk calculate position berdasarkan `expandDirection`
- Update `drawDropdown()` untuk render options dengan direction yang sama
- Hasil: Hitbox dan visual rendering sekarang perfectly aligned

### 2. **Window/Layout Symmetry Fix (Board Stretching)**
**Masalah**: Window layout tidak simetris - board bisa stretch dan terlihat jelek.

**Penyebab**: Board diposisikan left-aligned di available space, bukan centered. Jika tile size lebih kecil dari available space, ada gap yang tidak simetris.

**Solusi**:
```cpp
float boardWidth = tileSize * cols;
float boardHeight = tileSize * rows;
float offsetX = panelWidth + 20.0f + (availableWidth - boardWidth) / 2.0f;
float offsetY = 20.0f + (availableHeight - boardHeight) / 2.0f;
```
- Board sekarang centered dalam available space
- Layout sepenuhnya simetris (kiri-kanan dan atas-bawah padding equal)
- Tidak ada stretching atau asimetri

### 3. **Dropdown Menu Overlap Prevention (Dropdown Clash)**
**Masalah**: Dropdown menu clash dengan UI elements lain ketika dibuka.

**Penyebab**: Menu options selalu expand ke bawah, tidak considering available space.

**Solusi**:
- Tambah method `calculateExpandDirection()` untuk calculate apakah ada space untuk expand down/up
- Space calculation:
  - `spaceBelow = windowHeight - (pos.y + buttonSize.y)`
  - `spaceAbove = pos.y`
  - `expandDirection = 1` jika space below cukup, `-1` jika space above cukup
- Menu options sekarang smart-reposition:
  - Expand DOWN jika cukup space di bawah (default)
  - Expand UP jika tidak ada space di bawah tapi ada space di atas
  - Expand down sebagai fallback jika keduanya tight

### How to Test

1. **Test Hitbox Alignment**:
   - Load board dengan `Load Input`
   - Klik pada Algorithm dropdown - harus respond pada visual button area
   - Buka dropdown dan select opsi - harus bekerja dengan perfect alignment
   - Pastikan semua buttons (Load, Solve, dll) responsive pada visual position

2. **Test Layout Symmetry**:
   - Load board bervariasi ukuran (test-1 to test-6)
   - Amati board grid - harus centered dengan padding equal semua sisi
   - Tidak ada gaps atau stretching

3. **Test Dropdown Menu**:
   - Load board
   - Buka dropdown menu (Algorithm atau Heuristic)
   - Menu options tidak clash dengan playback controls di bawah
   - Dropdown otomatis reposition jika mendekati window edge
   - Click pada menu options harus bekerja dengan akurat

## Build Status
✅ Compile successful dengan no warnings/errors
✅ Semua SFML 3 API calls correct
✅ GUI executable ready: `bin/solver_gui`

## Files Modified
- `src/core/gui/GUI.cpp`: 
  - Enhanced Dropdown struct dengan expandDirection tracking
  - Improved optionBounds() calculation
  - Updated drawDropdown() untuk smart expansion direction
  - Added calculateExpandDirection() method
  - Board centering logic untuk symmetric layout
