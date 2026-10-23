#include "http_client.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <unistd.h>

#include <condition_variable>
#include <cstring>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

// Парсинг URL на компоненты
HttpClient::ParsedUrl HttpClient::parseUrl(const std::string& url) {
  ParsedUrl parsed;

  // Определяем протокол
  size_t protocol_end = url.find("://");
  if (protocol_end != std::string::npos) {
    parsed.protocol = url.substr(0, protocol_end);
    size_t host_start = protocol_end + 3;

    // Находим конец хоста (после хоста может быть порт, путь или параметры)
    size_t host_end = url.find('/', host_start);
    size_t port_pos = url.find(':', host_start);

    if (host_end == std::string::npos) {
      host_end = url.length();
    }

    // Извлекаем хост и порт если есть
    if (port_pos != std::string::npos && port_pos < host_end) {
      parsed.host = url.substr(host_start, port_pos - host_start);
      size_t port_end = (host_end < url.length()) ? host_end : url.length();
      std::string port_str = url.substr(port_pos + 1, port_end - port_pos - 1);
      parsed.port = std::stoi(port_str);
    } else {
      parsed.host = url.substr(host_start, host_end - host_start);
      parsed.port = (parsed.protocol == "https") ? 443 : 80;
    }

    // Извлекаем путь и параметры
    if (host_end < url.length()) {
      size_t query_pos = url.find('?', host_end);
      if (query_pos != std::string::npos) {
        parsed.path = url.substr(host_end, query_pos - host_end);
        parsed.query = url.substr(query_pos + 1);
      } else {
        parsed.path = url.substr(host_end);
      }
    } else {
      parsed.path = "/";
    }
  } else {
    // Если протокол не указан, используем HTTP по умолчанию
    parsed.protocol = "http";
    size_t host_end = url.find('/');

    if (host_end == std::string::npos) {
      parsed.host = url;
      parsed.path = "/";
    } else {
      parsed.host = url.substr(0, host_end);
      parsed.path = url.substr(host_end);
    }

    parsed.port = 80;
  }

  return parsed;
}

// Создание и подключение сокета к указанному хосту
void HttpClient::createSocket(const std::string& host, int port) {
  struct addrinfo hints, *result;

  // Обнуляем структуру hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;        // Используем IPv4
  hints.ai_socktype = SOCK_STREAM;  // Используем потоковый сокет (TCP)

  // Преобразуем доменное имя в IP-адрес
  if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints,
                  &result) != 0) {
    throw std::runtime_error("Ошибка разрешения доменного имени: " + host);
  }

  // Создаем сокет
  sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (sock < 0) {
    freeaddrinfo(result);
    throw std::runtime_error("Ошибка создания сокета");
  }

  // Подключаемся к серверу
  if (connect(sock, result->ai_addr, result->ai_addrlen) < 0) {
    freeaddrinfo(result);
    close(sock);
    sock = -1;
    throw std::runtime_error("Ошибка подключения к серверу: " + host);
  }

  freeaddrinfo(result);
}

// Инициализация SSL контекста
void HttpClient::initSSL() {
  // Загружаем криптографические алгоритмы и строки ошибок
  SSL_load_error_strings();
  OpenSSL_add_ssl_algorithms();

  // Создаем SSL контекст с методом TLS
  ssl_ctx = SSL_CTX_new(TLS_client_method());
  if (!ssl_ctx) {
    throw std::runtime_error("Ошибка создания SSL контекста");
  }
}

// Установка SSL соединения
void HttpClient::setupSSL(const std::string& host) {
  // Создаем SSL структуру
  ssl = SSL_new(ssl_ctx);
  if (!ssl) {
    throw std::runtime_error("Ошибка создания SSL структуры");
  }

  // Привязываем SSL к сокету
  if (SSL_set_fd(ssl, sock) == 0) {
    SSL_free(ssl);
    ssl = nullptr;
    throw std::runtime_error("Ошибка привязки SSL к сокету");
  }

  // Устанавливаем имя хоста для проверки сертификата
  if (SSL_set_tlsext_host_name(ssl, host.c_str()) == 0) {
    SSL_free(ssl);
    ssl = nullptr;
    throw std::runtime_error("Ошибка установки имени хоста");
  }

  // Устанавливаем SSL соединение
  if (SSL_connect(ssl) <= 0) {
    SSL_free(ssl);
    ssl = nullptr;
    throw std::runtime_error("Ошибка SSL handshake");
  }
}

// Отправка данных через сокет или SSL
void HttpClient::sendData(const std::string& data) {
  int bytes_sent;
  if (use_https) {
    bytes_sent = SSL_write(ssl, data.c_str(), data.length());
  } else {
    bytes_sent = send(sock, data.c_str(), data.length(), 0);
  }

  if (bytes_sent <= 0) {
    throw std::runtime_error("Ошибка отправки данных");
  }
}

// Получение данных из сокета или SSL
std::string HttpClient::receiveData() {
  std::string response;
  char buffer[4096];
  int bytes_received;

  while (true) {
    if (use_https) {
      bytes_received = SSL_read(ssl, buffer, sizeof(buffer));
    } else {
      bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    }

    if (bytes_received > 0) {
      response.append(buffer, bytes_received);
    } else if (bytes_received == 0) {
      // Соединение закрыто
      break;
    } else {
      throw std::runtime_error("Ошибка получения данных");
    }
  }

  return response;
}

// Конструктор
HttpClient::HttpClient()
    : sock(-1), ssl_ctx(nullptr), ssl(nullptr), use_https(false) {
  // Инициализируем OpenSSL
  SSL_library_init();
}

// Деструктор
HttpClient::~HttpClient() {
  // Закрываем SSL соединение и освобождаем ресурсы
  if (ssl) {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }

  // Освобождаем SSL контекст
  if (ssl_ctx) {
    SSL_CTX_free(ssl_ctx);
  }

  // Закрываем сокет
  if (sock >= 0) {
    close(sock);
  }

  // Очищаем OpenSSL
  EVP_cleanup();
}

// Выполнение HTTP GET запроса
std::string HttpClient::get(const std::string& url,
                            const std::map<std::string, std::string>& headers) {
  // Парсим URL
  ParsedUrl parsed = parseUrl(url);
  use_https = (parsed.protocol == "https");

  // Создаем и подключаем сокет
  createSocket(parsed.host, parsed.port);

  // Если используем HTTPS, инициализируем SSL
  if (use_https) {
    initSSL();
    setupSSL(parsed.host);
  }

  // Формируем путь с параметрами, если они есть
  std::string full_path = parsed.path;
  if (!parsed.query.empty()) {
    full_path += "?" + parsed.query;
  }

  // Формируем HTTP-запрос
  std::stringstream request;
  request << "GET " << full_path << " HTTP/1.1\r\n";
  request << "Host: " << parsed.host << "\r\n";
  request << "Connection: close\r\n";
  request << "User-Agent: Simple-CPP-HttpClient/1.0\r\n";

  // Добавляем заголовки
  for (const auto& header : headers) {
    request << header.first << ": " << header.second << "\r\n";
  }

  request << "\r\n";  // Конец заголовков

  // Отправляем запрос
  sendData(request.str());

  // Получаем ответ
  std::string response = receiveData();

  return response;
}

// Выполнение HTTP POST запроса
std::string HttpClient::post(const std::string& url, const std::string& data,
                             const std::map<std::string, std::string>& headers,
                             const std::string& content_type) {
  // Парсим URL
  ParsedUrl parsed = parseUrl(url);
  use_https = (parsed.protocol == "https");

  // Создаем и подключаем сокет
  createSocket(parsed.host, parsed.port);

  // Если используем HTTPS, инициализируем SSL
  if (use_https) {
    initSSL();
    setupSSL(parsed.host);
  }

  // Формируем HTTP-запрос
  std::stringstream request;
  request << "POST " << parsed.path << " HTTP/1.1\r\n";
  request << "Host: " << parsed.host << "\r\n";
  request << "Connection: close\r\n";
  request << "User-Agent: HttpClient/1.0\r\n";
  request << "Content-Type: " << content_type << "\r\n";
  request << "Content-Length: " << data.length() << "\r\n";

  // Добавляем заголовки
  for (const auto& header : headers) {
    request << header.first << ": " << header.second << "\r\n";
  }

  request << "\r\n";  // Конец заголовков
  request << data;    // Тело запроса

  // Отправляем запрос
  sendData(request.str());

  // Получаем ответ
  std::string response = receiveData();

  return response;
}

std::vector<std::string> HttpClient::extractLinks(const std::string& page) {
  std::vector<std::string> links;

  // Парсим HTML
  htmlDocPtr doc = htmlReadDoc(
      (const xmlChar*)page.c_str(), NULL, NULL,
      HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
  if (doc == NULL) {
    std::cerr << "Не удалось распарсить HTML" << std::endl;
    return links;
  }

  // Создаем контекст XPath
  xmlXPathContextPtr context = xmlXPathNewContext(doc);
  if (context == NULL) {
    std::cerr << "Ошибка создания XPath контекста" << std::endl;
    xmlFreeDoc(doc);
    return links;
  }

  // Выполняем XPath запрос для поиска всех ссылок
  xmlXPathObjectPtr result =
      xmlXPathEvalExpression((const xmlChar*)"//a/@href", context);
  if (result == NULL) {
    std::cerr << "Ошибка выполнения XPath запроса" << std::endl;
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);
    return links;
  }

  // Обрабатываем результаты
  xmlNodeSetPtr nodeset = result->nodesetval;
  if (nodeset != NULL) {
    for (int i = 0; i < nodeset->nodeNr; i++) {
      xmlChar* value =
          xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
      if (value != NULL) {
        std::string link((char*)value);
        links.push_back(link);
        xmlFree(value);
      }
    }
  }

  // Освобождаем ресурсы
  xmlXPathFreeObject(result);
  xmlXPathFreeContext(context);
  xmlFreeDoc(doc);

  return links;
}

std::string HttpClient::normalizeUrl(const std::string& baseUrl,
                                     const std::string& link) {
  // Здесь должна быть реализация нормализации URL
  // (преобразование относительных ссылок в абсолютные, удаление фрагментов и
  // т.д.) Для простоты примера возвращаем как есть
  if (link.find("http") == 0) {
    return link;
  } else {
    return baseUrl + link;
  }
}
std::string HttpClient::getDomain(const std::string& url) {
  size_t start = url.find("://") + 3;
  size_t end = url.find("/", start);
  if (end == std::string::npos) {
    return url.substr(start);
  }
  return url.substr(start, end - start);
}
void HttpClient::crawlWebsite(const std::string& startUrl, int maxPages,
                              int numThreads,
                              const std::string& outputXmlFile) {
  std::string domain = getDomain(startUrl);

  std::cout << "Начинаем обход сайта: " << startUrl << std::endl;
  std::cout << "Домен: " << domain << std::endl;

  std::set<std::string> visited;
  std::queue<std::string> toVisit;
  std::map<std::string, std::vector<std::string>> siteMap;
  toVisit.push(startUrl);

  std::mutex stateMutex;
  std::condition_variable cv;
  int activeWorkers = 0;

  auto worker = [&]() {
    HttpClient localClient;
    while (true) {
      std::string currentUrl;
      {
        std::unique_lock<std::mutex> lock(stateMutex);
        while (toVisit.empty() && activeWorkers > 0 &&
               visited.size() < static_cast<size_t>(maxPages)) {
          cv.wait(lock);
        }
        if (toVisit.empty() || visited.size() >= static_cast<size_t>(maxPages)) {
          cv.notify_all();
          return;
        }
        currentUrl = toVisit.front();
        toVisit.pop();
        if (visited.find(currentUrl) != visited.end()) {
          continue;
        }
        visited.insert(currentUrl);
        activeWorkers++;
      }

      std::cout << "Обрабатываем: " << currentUrl << std::endl;

      std::string page;
      try {
        page = localClient.get(currentUrl);
      } catch (const std::exception& e) {
        std::cerr << "Ошибка загрузки " << currentUrl << ": " << e.what()
                  << std::endl;
      }

      std::vector<std::string> links =
          page.empty() ? std::vector<std::string>{} : extractLinks(page);

      {
        std::unique_lock<std::mutex> lock(stateMutex);
        std::vector<std::string>& children = siteMap[currentUrl];
        for (const std::string& link : links) {
          std::string normalizedLink = normalizeUrl(currentUrl, link);

          // Проверяем, что ссылка ведет на тот же домен
          if (getDomain(normalizedLink) == domain) {
            children.push_back(normalizedLink);
            if (visited.find(normalizedLink) == visited.end() &&
                visited.size() < static_cast<size_t>(maxPages)) {
              toVisit.push(normalizedLink);
            }
          }
        }
        activeWorkers--;
        cv.notify_all();
      }
    }
  };

  std::vector<std::thread> workers;
  for (int i = 0; i < numThreads; ++i) {
    workers.emplace_back(worker);
  }
  for (std::thread& t : workers) {
    t.join();
  }

  std::cout << "Обход завершен. Посещено страниц: " << visited.size()
            << std::endl;
  std::cout << "Список посещенных страниц:" << std::endl;
  for (const std::string& url : visited) {
    std::cout << "  " << url << std::endl;
  }

  saveSiteMapXml(startUrl, siteMap, outputXmlFile);
  std::cout << "Карта сайта сохранена в файл: " << outputXmlFile << std::endl;
}

void HttpClient::saveSiteMapXml(
    const std::string& startUrl,
    const std::map<std::string, std::vector<std::string>>& siteMap,
    const std::string& filename) {
  xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
  xmlNodePtr root = xmlNewNode(NULL, (const xmlChar*)"sitemap");
  xmlDocSetRootElement(doc, root);

  std::set<std::string> expanded;
  std::function<void(xmlNodePtr, const std::string&)> buildTree =
      [&](xmlNodePtr parentNode, const std::string& url) {
        xmlNodePtr pageNode =
            xmlNewChild(parentNode, NULL, (const xmlChar*)"page", NULL);
        xmlNewProp(pageNode, (const xmlChar*)"url", (const xmlChar*)url.c_str());

        // Каждый узел разворачивается только один раз, чтобы циклические
        // ссылки между страницами не привели к бесконечной рекурсии
        if (!expanded.insert(url).second) {
          return;
        }

        auto it = siteMap.find(url);
        if (it != siteMap.end()) {
          for (const std::string& child : it->second) {
            buildTree(pageNode, child);
          }
        }
      };

  buildTree(root, startUrl);

  xmlSaveFormatFileEnc(filename.c_str(), doc, "UTF-8", 1);
  xmlFreeDoc(doc);
}
