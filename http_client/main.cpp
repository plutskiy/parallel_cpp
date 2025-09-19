#include <iostream>
#include <regex>
#include "http_client.h"

std::string extractByRegex(const std::string& html) {
    // Регулярное выражение для поиска тега title
    // Игнорируем регистр и разрешаем пробелы вокруг знаков равенства
    std::regex titleRegex("<title[^>]*>(.*?)</title>",
                         std::regex_constants::icase | std::regex_constants::ECMAScript);

    std::smatch match;
    if (std::regex_search(html, match, titleRegex) && match.size() > 1) {
        return match.str(1); // Возвращаем содержимое тега title
    }

    return "Title not found"; // Если тег title не найден
}
int main() {
  HttpClient client;

  // Простой GET запрос
  //std::string response = client.get("https://habr.com/ru/articles/944742/");
  //std::string title = extractByRegex(response);
  //std::cout << title;
  client.crawlWebsite("https://habr.com/ru/articles/944742/");

  // GET запрос с заголовками
  // std::map<std::string, std::string> headers;
  // headers["Authorization"] = "Bearer token123";
  // headers["Accept"] = "application/json";
  // std::string response = client.get("https://api.example.com/data", headers);

  // POST запрос с данными формы
  // std::string data = "username=test&password=secret";
  // std::string response = client.post("http://example.com/login", data);

  // POST запрос с JSON
  // std::string json_data = "{\"name\": \"John\", \"age\": 30}";
  // std::map<std::string, std::string> headers;
  // headers["Content-Type"] = "application/json";
  // std::string response = client.post("https://api.example.com/users", json_data, headers, "application/json");
}