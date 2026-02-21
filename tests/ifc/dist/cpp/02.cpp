#include <flow/io>


template <typename T = int>
struct result {
    enum tags { Ok, Err };
    tags tag;
    union {
        T Ok;
    } value;
    constexpr operator tags() const noexcept { return tag; }
    result& operator=(tags t) noexcept { tag = t; return *this; }
    template <typename ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789>
    result& operator=(ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789&& val) {
        using U = std::decay_t<ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ123456789>;
        if (tag == tags::Ok && std::is_same_v<U, T>) { value.Ok = std::forward<T>(val); }
        else { throw std::runtime_error("invalid assignment to tagged union"); }
        return *this;
    }
};


using namespace flow;

int main(){
    result<int> r;

    r = result<int>::Err;

    switch (r) {
        case result<>::Ok:
            println("Ok = ", r.value.Ok);
            break;
        case result<>::Err:
            println("Err");
            break;
    }

    return 0;
}