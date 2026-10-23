#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

std::string toLower(const std::string& str) {
  std::string result;
  for (char c : str) {
    result += std::tolower(c);
  }
  return result;
}

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

struct WordCount {
  std::string word;
  int count;
};

bool compareWordCount(const WordCount& a, const WordCount& b) {
  return a.count > b.count;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Использование: " << argv[0] << " <файл> [количество слов]"
              << std::endl;
    return 1;
  }
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
    word = cleanWord(word);

    word = toLower(word);

    if (word.empty()) {
      continue;
    }

    wordCountMap[word]++;
  }

  file.close();

  std::vector<WordCount> wordCounts;
  for (const auto& pair : wordCountMap) {
    wordCounts.push_back({pair.first, pair.second});
  }

  std::sort(wordCounts.begin(), wordCounts.end(), compareWordCount);

  // Выводим результаты
  std::cout << "Топ-" << topN << " самых частых слов в файле '" << filename
            << "':" << std::endl;
  std::cout << std::setw(20) << std::left << "Слово" << "Количество"
            << std::endl;
  std::cout << std::string(30, '-') << std::endl;

  int limit = std::min(topN, static_cast<int>(wordCounts.size()));
  for (int i = 0; i < limit; i++) {
    std::cout << std::setw(20) << std::left << wordCounts[i].word
              << wordCounts[i].count << std::endl;
  }

  return 0;
}
