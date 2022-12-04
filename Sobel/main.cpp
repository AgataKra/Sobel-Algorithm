#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

using namespace std;

struct type {
    uint16_t bfType;
};

struct file_header {
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct picture_header {
    uint32_t biSize;
    uint32_t biWidth;
    uint32_t biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    uint32_t biXpelsPerMeter;
    uint32_t biYpelsPerMeter;
    uint32_t biCrlUses;
    uint32_t biCrlImportant;
};

struct mask {
    int value[9][3][3];
};

void readFileHeader(ifstream& file, type& signature, file_header& F_header);
void readPictureHeader(ifstream& file, picture_header& P_header);
void printInfo(type signature, file_header BMP_H, picture_header P_header);
void transformWithSobelAlgorithm(uint8_t image_RGB_1[], uint8_t image_RGB_2[], uint8_t image_RGB_3[], unsigned int column_number, unsigned int number_of_zeroes, uint8_t new_image[], unsigned int row, int choice, mask matrix);
void printhelp();
void readMatrix(mask& matrix, unsigned int i);
void copyHeaders(ofstream& result, ifstream& file, file_header F_header);

int main() {
    file_header F_header = {};
    picture_header P_header = {};
    type signature = {};
    string file_name, result_name;
    int choice;
    printhelp();

    cout << "Podaj nazwe pliku do przetworzenia: ";
    getline(cin, file_name);
    ifstream file;
    ofstream result;
    file.open(file_name, ios::binary);

    if (!file) {
        cout << "Plik nie zostal otworzony.";
        return 0;
    }

    readFileHeader(file, signature, F_header);
    readPictureHeader(file, P_header);
    printInfo(signature, F_header, P_header);

    file.close();

    cout << "Podaj nazwe pliku wyjsciowego: ";
    getline(cin, result_name);

    cout << "Wybierz tryb pracy: " << endl;
    cout << "1. Wczytaj caly obraz do pamieci RAM (w tablicy alokowanej dynamicznie)." << endl;
    cout << "2. Wczytuj fragmenty obrazu po kolei." << endl;
    cin >> choice;

    unsigned int number_of_zeroes = (P_header.biWidth * 3) % 4;
    if (number_of_zeroes == 0)
        number_of_zeroes = 0;
    else
        number_of_zeroes = 4 - number_of_zeroes;
    unsigned int row_number = P_header.biHeight;                                                                                                                                    // 2 add rows for borders
    unsigned int column_number = P_header.biWidth * 3 + number_of_zeroes;                                                                                   //6 columns for borders (3 each side)
    uint8_t border_value = 0;
    mask matrix = { 0 };
    for (unsigned int i = 1; i < 9; i++)
        readMatrix(matrix, i);

    uint8_t* image_RGB_1 = new uint8_t[column_number];
    uint8_t* image_RGB_2 = new uint8_t[column_number];
    uint8_t* image_RGB_3 = new uint8_t[column_number];

    file.open(file_name, ios::binary);
    result.open(result_name, ios::binary);
    copyHeaders(result, file, F_header);

    if (choice == 1) {
        uint8_t* image_RGB = new uint8_t[column_number * row_number];
        uint8_t* new_image = new uint8_t[column_number * row_number];
        for (unsigned int i = 0; i < column_number * row_number; i++)
            new_image[i] = uint8_t(0);

        file.seekg(F_header.bfOffBits);                                                                                                                                                         //start reading from beginning of picture
        file.read((char*)image_RGB, F_header.bfSize - F_header.bfOffBits);
        file.close();
        result.seekp(F_header.bfOffBits);

        for (unsigned int i = 0; i < row_number; i++) {
            if (i == 0)
                for (unsigned int j = 0; j < column_number; j++)
                    image_RGB_1[j] = border_value;
            else
                memcpy(image_RGB_1, image_RGB + (i - 1) * column_number, sizeof(uint8_t) * column_number);

            if (i == row_number - 1)
                for (int j = 0; j < column_number; j++)
                    image_RGB_3[j] = border_value;
            else
                memcpy(image_RGB_3, image_RGB + (i + 1) * column_number, sizeof(uint8_t) * column_number);

            memcpy(image_RGB_2, image_RGB + i * column_number, sizeof(uint8_t) * column_number);
            transformWithSobelAlgorithm(image_RGB_1, image_RGB_2, image_RGB_3, column_number, number_of_zeroes, new_image, i, choice, matrix);
        }
        result.write((char*)new_image, F_header.bfSize - F_header.bfOffBits);
        result.close();

        delete[] new_image;
        delete[] image_RGB_1;
        delete[] image_RGB_2;
        delete[] image_RGB_3;
        delete[] image_RGB;
    }

    else if (choice == 2) {
        
        uint8_t* new_image = new uint8_t[column_number];
        for (unsigned int i = 0; i < column_number; i++)
            new_image[i] = uint8_t(0);

        
        result.seekp(F_header.bfOffBits);
        file.seekg(F_header.bfOffBits);

        for (unsigned int i = 0; i < row_number; i++) {
            if (i == 0)
                for (unsigned int j = 0; j < column_number; j++)
                    image_RGB_1[j] = border_value;
            else {
                file.seekg(F_header.bfOffBits + sizeof(uint8_t) * column_number * (i - 1));
                file.read((char*)image_RGB_1, sizeof(uint8_t) * column_number);
            }

            if (i == row_number - 1)
                for (unsigned int j = 0; j < column_number; j++)
                    image_RGB_3[j] = border_value;
            else {
                file.seekg(F_header.bfOffBits + sizeof(uint8_t) * column_number * (i + 1));
                file.read((char*)image_RGB_3, sizeof(uint8_t) * column_number);
            }
            file.seekg(F_header.bfOffBits + sizeof(uint8_t) * column_number * i);
            file.read((char*)image_RGB_2, sizeof(uint8_t) * column_number);
            transformWithSobelAlgorithm(image_RGB_1, image_RGB_2, image_RGB_3, column_number, number_of_zeroes, new_image, i, choice, matrix);
            result.write((char*)new_image, sizeof(uint8_t) * column_number);
        }

        file.close();
        result.close();

        delete[] new_image;
        delete[] image_RGB_1;
        delete[] image_RGB_2;
        delete[] image_RGB_3;
    }


    return 0;
}

void printhelp() {
    cout << "Transformacja bitmapy 24-bitowej za pomoca algorytmu Sobela." << endl <<
        "Filtry (tablice) sa przechowywane w plikach tekstowych, po jednej wartosci w linijce." << endl <<
        "Nazwy plikow to S + numer maski." << endl << endl;
}

void readFileHeader(ifstream& file, type& signature, file_header& F_header) {
    file.read((char*)&signature, sizeof(type));
    file.read((char*)&F_header, sizeof(file_header));
}

void readPictureHeader(ifstream& file, picture_header& P_header) {
    file.read((char*)&P_header, sizeof(picture_header));
}

void copyHeaders(ofstream& result, ifstream& file, file_header F_header) {
    file.seekg(0);
    char* buffer = new char[F_header.bfOffBits];
    file.read(buffer, F_header.bfOffBits);
    result.write(buffer, F_header.bfOffBits);
    delete[]buffer;
}

void printInfo(type signature, file_header F_header, picture_header P_header) {
    cout << endl;
    cout << "Typ: " << signature.bfType << endl;
    cout << "Rozmiar: " << F_header.bfSize << endl;
    cout << "Zarezerwowane 1: " << F_header.bfReserved1 << endl;
    cout << "Zarezerwowane 2: " << F_header.bfReserved2 << endl;
    cout << "Pozycja danych w pliku: " << F_header.bfOffBits << endl;
    cout << "Rozmiar naglowka informacyjnego: " << P_header.biSize << endl;
    cout << "Szerokosc w pikselach: " << P_header.biWidth << endl;
    cout << "Wysokosc w pikselach: " << P_header.biHeight << endl;
    cout << "Liczba platow: " << P_header.biPlanes << endl;
    cout << "Liczba bitow na piksel: " << P_header.biBitCount << endl;
    cout << "Algorytm kompresji: " << P_header.biCompression << endl;
    cout << "Rozmiar rysunku: " << P_header.biSizeImage << endl;
    cout << "Rozdzielczosc pozioma: " << P_header.biXpelsPerMeter << endl;
    cout << "Rozdzielczosc pionowa: " << P_header.biYpelsPerMeter << endl;
    cout << "Liczba kolorow w palecie: " << P_header.biCrlUses << endl;
    cout << "Liczba waznych kolorow w palecie: " << P_header.biCrlImportant << endl << endl;
}

void transformWithSobelAlgorithm(uint8_t image_RGB_1[], uint8_t image_RGB_2[], uint8_t image_RGB_3[], unsigned int column_number, unsigned int number_of_zeroes, uint8_t new_image[], unsigned int row, int choice, mask matrix) {
    for (unsigned int column = 0; column < column_number; column++) {
        int convolute_sum[8] = { 0 };
        int pixel_value = 0;
        uint8_t convert = 0;
        for (int matrix_number = 1; matrix_number <= 8; matrix_number++) {

            convolute_sum[matrix_number - 1] += matrix.value[matrix_number][0][1] * image_RGB_1[column];
            convolute_sum[matrix_number - 1] += matrix.value[matrix_number][1][1] * image_RGB_2[column];
            convolute_sum[matrix_number - 1] += matrix.value[matrix_number][2][1] * image_RGB_3[column];
            if (column - 3 >= 0) {
                convolute_sum[matrix_number - 1] += matrix.value[matrix_number][0][0] * image_RGB_1[column - 3];
                convolute_sum[matrix_number - 1] += matrix.value[matrix_number][1][0] * image_RGB_2[column - 3];
                convolute_sum[matrix_number - 1] += matrix.value[matrix_number][2][0] * image_RGB_3[column - 3];
            }
            if (column + 3 <= column_number) {
                convolute_sum[matrix_number - 1] += matrix.value[matrix_number][0][2] * image_RGB_1[column + 3];
                convolute_sum[matrix_number - 1] += matrix.value[matrix_number][1][2] * image_RGB_2[column + 3];
                convolute_sum[matrix_number - 1] += matrix.value[matrix_number][2][2] * image_RGB_3[column + 3];
            }
            if (convolute_sum[matrix_number - 1] < 0)
                convolute_sum[matrix_number - 1] = abs(convolute_sum[matrix_number - 1]);
            if (convolute_sum[matrix_number - 1] > 255)
                convolute_sum[matrix_number - 1] = 255;

            pixel_value += convolute_sum[matrix_number - 1] / 8;
        }
        convert = (uint8_t)pixel_value;

        if (choice == 1)
            new_image[row * column_number + column] = convert;
        if (choice == 2)
            new_image[column] = convert;
    }
}


void readMatrix(mask& matrix, unsigned int i) {
    string matrix_name = "S" + to_string(i) + ".txt";
    ifstream values;
    values.open(matrix_name);
    values >> matrix.value[i][0][0] >> matrix.value[i][0][1] >> matrix.value[i][0][2] >>
        matrix.value[i][1][0] >> matrix.value[i][1][1] >> matrix.value[i][1][2] >>
        matrix.value[i][2][0] >> matrix.value[i][2][1] >> matrix.value[i][2][2];
}