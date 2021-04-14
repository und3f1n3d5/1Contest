#include <iostream>
#include <vector>
#include "lodepng.h"



/*
           start
            \/
           0 | 1 | 2
           ----------
           3 | 4 | 5       
           ----------
           6 | 7 | 8
 */

using namespace std;
const size_t pixel_size = 4;
uint32_t height, width;


void ProcessImageAfterRead(const vector<unsigned char> &image, uint32_t width, uint32_t height, vector<vector<bool> > &res) {
    uint64_t k = 0;
    for (uint32_t i = 0; i < height * pixel_size; i += pixel_size) {
        vector<bool> row;
        for (uint32_t j = 0; j < width * pixel_size; j += pixel_size) {
            if (image[k * pixel_size] == 0) {
                row.emplace_back(false);
            } else {
                row.emplace_back(true);
            }
            ++k;
        }
        res.emplace_back(row);
    }
}

void ReadImage(string& filename, uint32_t width, uint32_t height, vector<vector<bool> > &res) {
    vector<unsigned char> image; //the raw pixels

    //decode
    unsigned error = lodepng::decode(image, width, height, filename);

    //if there's an error, display it
    if(error) {
        std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
        throw;
    }

    //the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...
    ProcessImageAfterRead(image, width, height, res);
}

void ProcessImageBeforeWrite(vector<unsigned char> &image, uint32_t width, uint32_t height, const vector<vector<bool> > &res) {
    uint64_t k = 0;
    image.resize(width * height * pixel_size);
    for (uint32_t i = 0; i < height; ++i) {
        for (uint32_t j = 0; j < width; ++j) {
            if (res[i][j]) {
                image[k] = 255;
                image[k + 1] = 255;
                image[k + 2] = 255;
            } else {
                image[k] = 0;
                image[k + 1] = 0;
                image[k + 2] = 0;
            }
            image[k + 3] = 255;
            ++k;
        }
    }
}

void encodeOneStep(const char* filename, std::vector<unsigned char>& image, unsigned width, unsigned height) {
    //Encode the image
    unsigned error = lodepng::encode(filename, image, width, height);

    //if there's an error, display it
    if(error) {
        std::cout << "encoder error " << error << ": "<< lodepng_error_text(error) << std::endl;
        throw;
    }
}

void WriteImage(uint32_t width, uint32_t height, const vector<vector<bool> > &res) {
    vector<unsigned char> image;
    ProcessImageBeforeWrite(image, width, height, res);
    encodeOneStep("result_image.png", image, width, height);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


uint32_t square_height, line_width, square_width;
uint32_t start_x, start_y; // хотим найти верхнюю левую точку сетки (на вертикальной линии)
const size_t field_size = 3;

enum Figure{
    Empty,
    Zero,
    Cross
};

pair<uint32_t, uint32_t> JumpToSquareFromStart(const vector<vector<bool> > &image, size_t n) {
    return make_pair(
            start_x + (line_width + square_width) * (n % field_size),
            start_y + (line_width + square_height) * (n / field_size) + square_height / 2 // хотим приходить в середину клетки
            );
}

Figure DetectFigure(const vector<vector<bool> > &image, uint32_t x, uint32_t y) {
    bool line_crossed = false;
    for (uint32_t i=x; i > x - square_width / 2; --i) {
        if (!image[y][i]) {
            line_crossed = true;
        }
    }
    if (line_crossed && image[y][x - square_width / 2]) {
        return Zero;
    }
    if (!image[y][x - square_width / 2] || !image[y][x - square_width / 2 - 1]) {
        return Cross;
    }
    return Empty;
}

pair<uint32_t, uint32_t> GetCenterOfSquare(size_t n) {
    return make_pair(
            start_x + (line_width + square_width) * (n % field_size) - square_width / 2,
            start_y + (line_width + square_height) * (n / field_size) + square_height / 2
            );
}

uint32_t FindSquareHeight(const vector<vector<bool> > &image, uint32_t x, uint32_t y) {
    --x;
    for (uint32_t i=y; i < height; ++i) {
        if (!image[i][x] && !image[i][x + 1])
            return (i - y);
    }
    std::cout <<  "square height not found";
    throw;
}

uint32_t FindSquareWidth(const vector<vector<bool> > &image, uint32_t x, uint32_t y) {
    ++x;
    for (uint32_t i=x; i < width; ++i) {
        if (!image[y][i])
            return (i - x);
    }
    std::cout <<  "square width not found";
    throw;
}

uint32_t FindLineWidth(const vector<vector<bool> > &image, uint32_t x, uint32_t y) {
    for (uint32_t i=x; i < width; ++i) {
        if (image[y][i])
            return (i - x);
    }
    std::cout <<  "line width not found";
    throw;
}

bool CheckIfGrid(const vector<vector<bool> > &image, uint32_t x, uint32_t y) { // проверяет, является ли пиксель с координатами (x, y) крайнем пикселем сетки
    --x;
    for (uint32_t i=y; i < height; ++i) {
        if (image[i][x] && !image[i][x + 1]) {
            continue;
        }
        if (i - y > 3 && !image[i][x] && !image[i][x + 1]) { // для отрисовки крестика нужно минимум 3 пикселя в высоту => высота клетки должна быть больше 3
            return true;
        } else {
            return false;
        }
    }
    std::cout <<  "very strange picture"; // поскольку мы идем сверху, то мы должны пересечь хотя бы горизонтальную линию
    throw;
}

int main() {
    // запросим ввод
    string file;
    cout << "Please, enter filename, width and height of image: ";
    cin >> file >> width >> height;
    vector<vector<bool> > image; // черное - false, белое - true
    ReadImage(file, width, height, image);
    //WriteImage(width, height, image);

    // находим стартовые координаты
    bool br = false;
    for (uint32_t i = 0; i < width; ++i) {
        for (uint32_t j = 0; j < height; ++j) {
            if (!image[i][j]) {
                if (CheckIfGrid(image, j, i)) {
                    start_x = j - 1;
                    start_y = i;
                    square_height = FindSquareHeight(image, j, i);
                    line_width = FindLineWidth(image, j, i);
                    square_width = FindSquareWidth(image, j + line_width, i);
                    br = true;
                    break;
                }
            }
        }
        if (br) {
            break;
        }
    }

    // по картинке определяем фигуры и наносим их на поле
    vector<vector <Figure> > field(field_size, vector<Figure> (field_size, Empty));
    for (size_t i = 0; i < 9; ++i) {
        pair<uint32_t, uint32_t> xy = JumpToSquareFromStart(image, i);
        field[i / field_size][i % field_size] = DetectFigure(image, xy.first, xy.second);
    }

    // проверяем выигрышные комбинации
    size_t id1=0, id2=0; // номера клеток между которыми будет проходить прямая
    if (field[0][0] == field[1][1] && field[1][1] == field[2][2] && field[0][0] != Empty) {
        id1 = 0;
        id2 = 8;
    }
    if (field[0][1] == field[1][1] && field[1][1] == field[2][1] && field[1][1] != Empty) {
        id1 = 1;
        id2 = 7;
    }
    if (field[1][0] == field[1][1] && field[1][1] == field[1][2] && field[1][1] != Empty) {
        id1 = 3;
        id2 = 5;
    }
    if (field[0][2] == field[1][1] && field[1][1] == field[2][0] && field[1][1] != Empty) {
        id1 = 2;
        id2 = 6;
    }
    if (field[0][0] == field[0][1] && field[0][1] == field[0][2] && field[0][0] != Empty) {
        id1 = 0;
        id2 = 2;
    }
    if (field[0][0] == field[1][0] && field[1][0] == field[2][0] && field[0][0] != Empty) {
        id1 = 0;
        id2 = 6;
    }
    if (field[0][2] == field[1][2] && field[1][2] == field[2][2] && field[2][2] != Empty) {
        id1 = 2;
        id2 = 8;
    }
    if (field[2][0] == field[2][1] && field[2][1] == field[2][2] && field[2][2] != Empty) {
        id1 = 6;
        id2 = 8;
    }
    if (id1 ==0 && id2 == 0) {
        cout << "NO WINNER";
        return 0;
    }

    // начертим линию
    uint32_t winning_line_width = max(uint32_t(1), line_width / 4);
    pair<uint32_t, uint32_t> end1 = GetCenterOfSquare(id1), end2 = GetCenterOfSquare(id2);
    for (auto i = end1; i.first <= end2.first; ++i.first, ++i.second) {
        for (int32_t j = -winning_line_width; j <= winning_line_width; ++j) {
            image[i.first + j][i.second + j] = false;
        }
    }

    WriteImage(width, height, image);
    return 0;
}
