#include <iostream>
#include <flow/io>
struct result {
    enum tags { Ok, Err };
    tags tag;
    union {
        int Ok;
        int Err;
    } value;
    constexpr operator tags() const noexcept { return tag; }
    result& operator=(tags t) noexcept { tag = t; return *this; }
    template <typename T>
    result& operator=(T&& val) noexcept {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, int>) { value.Ok = std::forward<T>(val); }
        if constexpr (std::is_same_v<U, int>) { value.Err = std::forward<T>(val); }
        return *this;
    }
};


using namespace flow;

int main(){

    result r;

    r = result::Err;
    r = -1;

    switch (r) {
        case result::Ok:
            println("Ok = ", r.value.Ok);
            break;
        case result::Err:
            println("Err = ", r.value.Err);
            break;
    }

    return 0;
}