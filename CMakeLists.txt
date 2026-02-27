cmake_minimum_required(VERSION 3.20)
project(smtp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# “»ћ„ј—ќ¬ќ ¬»ћ» ј™ћќ все ≥нше, щоб воно не гальмувало комп≥л€ц≥ю
# ≥ не просило Qt6 чи ≥нш≥ б≥бл≥отеки:
# add_subdirectory(proto)
# add_subdirectory(server)
# add_subdirectory(mua)

# =========================================
# “¬ќя «ќЌј (MIME “ј “≈—“»)
# =========================================

# ƒодаЇмо тв≥й тест + твоњ робоч≥ файли + файли логера
add_executable(mime_test 
    tests/test_mime.cpp 
    common/mime/MimeBuilder.cpp
    
    # —юди треба вписати .cpp файли логера, €к≥ Ї у ваш≥й папц≥ logger.
    # —уд€чи з твого минулого скр≥ншоту, вони називаютьс€ приблизно так:
    # якщо Ї ≥нш≥ .cpp (наприклад ConsoleStrategy.cpp), розкоментуй ≥ додай:
    # якщо Ї ≥нш≥ .cpp (наприклад ConsoleStrategy.cpp), розкоментуй ≥ додай:
    # logger/ConsoleStrategy.cpp
 "common/mime/MimeParser.cpp" "tests/MockLogger.h" "common/mime/MimeDecoder.h" "common/mime/MimeDecoder.cpp")

# ¬казуЇмо папки, де лежать .h файли
target_include_directories(mime_test PRIVATE common)
target_include_directories(mime_test PRIVATE logger)