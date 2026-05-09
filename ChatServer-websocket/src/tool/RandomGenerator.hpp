#pragma once
#include <random>
#include <string>
#include <sstream>

class RandomGenerator {
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> dist;

public:
    RandomGenerator() : gen(rd()), dist(0, 9) {}

    std::string generateVerificationCode(int length = 6) {
        std::stringstream ss;
        for (int i = 0; i < length; ++i) {
            ss << dist(gen);
        }
        return ss.str();
    }

    std::string generateRandomString(int length) {
        const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::uniform_int_distribution<> charDist(0, chars.size() - 1);
        std::stringstream ss;
        for (int i = 0; i < length; ++i) {
            ss << chars[charDist(gen)];
        }
        return ss.str();
    }

    int generateRandomInt(int min, int max) {
        std::uniform_int_distribution<> intDist(min, max);
        return intDist(gen);
    }
};