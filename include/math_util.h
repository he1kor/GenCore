#pragma once

template <typename T> int sign(T value) {
    return (T(0) < value) - (value < T(0));
}