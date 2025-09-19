#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <iomanip>

// Функция для приведения строки к нижнему регистру
std::string toLower(const std::string& str) {
    std::string result;
    for (char c : str) {
        result += std::tolower(c);
    }
    return result;
}

// Функция для очистки слова от знаков препинания
std::string cleanWord(std::string word) {
    // Удаляем знаки препинания в начале слова
    while (!word.empty() && std::ispunct(word.front())) {
        word.erase(0, 1);
    }
    
    // Удаляем знаки препинания в конце слова
    while (!word.empty() && std::ispunct(word.back())) {
        word.pop_back();
    }
    
    return word;
}

// Структура для хранения слова и его частоты
struct WordCount {
    std::string word;
    int count;
};

// Функция для сравнения двух WordCount объектов (для сортировки)
bool compareWordCount(const WordCount& a, const WordCount& b) {
    return a.count > b.count;
}

int main(int argc, char* argv[]) {
    // Проверяем аргументы командной строки
    if (argc < 2) {
        std::cerr << "Использование: " << argv[0] << " <файл> [количество слов]" << std::endl;
        return 1;
    }
    
    // Получаем имя файла и количество слов для вывода
    std::string filename = argv[1];
    int topN = 10;
    if (argc > 2) {
        topN = std::stoi(argv[2]);
    }
    
    // Открываем файл
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл: " << filename << std::endl;
        return 1;
    }
    
    // Карта для подсчета слов
    std::map<std::string, int> wordCountMap;
    std::string word;
    
    // Читаем слова из файла
    while (file >> word) {
        // Очищаем слово от знаков препинания
        word = cleanWord(word);
        
        // Приводим к нижнему регистру
        word = toLower(word);
        
        // Пропускаем пустые слова
        if (word.empty()) {
            continue;
        }
        
        // Увеличиваем счетчик для этого слова
        wordCountMap[word]++;
    }
    
    // Закрываем файл
    file.close();
    
    // Преобразуем map в вектор для сортировки
    std::vector<WordCount> wordCounts;
    for (const auto& pair : wordCountMap) {
        wordCounts.push_back({pair.first, pair.second});
    }
    
    // Сортируем вектор по убыванию частоты
    std::sort(wordCounts.begin(), wordCounts.end(), compareWordCount);
    
    // Выводим результаты
    std::cout << "Топ-" << topN << " самых частых слов в файле '" << filename << "':" << std::endl;
    std::cout << std::setw(20) << std::left << "Слово" << "Количество" << std::endl;
    std::cout << std::string(30, '-') << std::endl;
    
    int limit = std::min(topN, static_cast<int>(wordCounts.size()));
    for (int i = 0; i < limit; i++) {
        std::cout << std::setw(20) << std::left << wordCounts[i].word 
                  << wordCounts[i].count << std::endl;
    }
    
    return 0;
}
