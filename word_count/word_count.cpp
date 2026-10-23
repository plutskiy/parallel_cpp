#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
#include <string>
#include <vector>

constexpr int kBlockSize = 10000;  // слов в одном блоке
constexpr int kTopDefault = 100;

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

void countBlock(const std::vector<std::string>& block,
                 std::map<std::string, int>& wordCountMap, std::mutex& mtx) {
  std::map<std::string, int> localCounts;
  for (const std::string& raw : block) {
    std::string word = toLower(cleanWord(raw));
    if (!word.empty()) {
      localCounts[word]++;
    }
  }

  std::lock_guard<std::mutex> lock(mtx);
  for (const auto& [word, count] : localCounts) {
    wordCountMap[word] += count;
  }
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Использование: " << argv[0] << " <файл> [количество слов]"
              << std::endl;
    return 1;
  }
  std::string filename = argv[1];
  int topN = argc > 2 ? std::stoi(argv[2]) : kTopDefault;

  // Открываем файл
  std::ifstream file(filename);
  if (!file.is_open()) {
    std::cerr << "Не удалось открыть файл: " << filename << std::endl;
    return 1;
  }

  std::map<std::string, int> wordCountMap;
  std::mutex mtx;
  std::vector<std::thread> threads;

  std::string word;
  std::vector<std::string> block;
  block.reserve(kBlockSize);
  while (file >> word) {
    block.push_back(word);
    if (static_cast<int>(block.size()) == kBlockSize) {
      threads.emplace_back(countBlock, std::move(block), std::ref(wordCountMap),
                            std::ref(mtx));
      block.clear();
      block.reserve(kBlockSize);
    }
  }
  if (!block.empty()) {
    threads.emplace_back(countBlock, std::move(block), std::ref(wordCountMap),
                          std::ref(mtx));
  }

  for (auto& t : threads) {
    t.join();
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
