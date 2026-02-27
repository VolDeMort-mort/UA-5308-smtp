cmake_minimum_required(VERSION 3.20)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Частина Ані (Основна структура проекту) ---
find_package(Boost) # Якщо Boost не знайдено, це може бути проблемою пізніше, але поки ок

# Підключаємо модуль Ані (тут лежить Base64)
add_subdirectory(base64)

# Опції для білда (можна вимкнути SERVER/MUA, якщо вони заважають, змінивши ON на OFF)
option(BUILD_SERVER "Build the SMTP server" OFF)
option(BUILD_MUA "Build the Mail User Agent" OFF)

# Підключаємо інші папки, якщо опції увімкнені
add_subdirectory(proto)

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_MUA)
    add_subdirectory(mua)
endif()


# =========================================
# ТВОЯ ЗОНА (MIME ТА ТЕСТИ)
# =========================================

# Додаємо твій тест. 
# ВАЖЛИВО: Я додав сюди шлях до коду Ані, щоб ти міг його використовувати.
add_executable(mime_test 
    tests/test_mime.cpp 
    common/mime/MimeBuilder.cpp
    common/mime/MimeParser.cpp
    common/mime/MimeDecoder.cpp
    
    # Додаємо файли для тестів (заміни MockLogger на реальний, коли буде готовий)
    tests/MockLogger.h
)

# Вказуємо папки, де лежать .h файли
target_include_directories(mime_test PRIVATE common)
target_include_directories(mime_test PRIVATE logger)

# !!! ІНТЕГРАЦІЯ !!!
# Додаємо шлях до папки base64, щоб ти міг писати #include "Base64.h"
target_include_directories(mime_test PRIVATE base64)

# Якщо Аня зробила бібліотеку в папці base64 (target_link_libraries), 
# то її треба буде слінковати. Але поки просто додамо include.