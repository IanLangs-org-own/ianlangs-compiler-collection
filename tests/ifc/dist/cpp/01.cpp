#include <flow/types>
#include <flow/io> 
#include <flow/math>
using namespace flow;

template <typename T>
using Matrix  = flow::matrix<T>;

using StrVector = flow::vector<str>;

template <typename T>
any make_any(T x){
    return x;
}

int main(){
    str msg = "start";
    println(msg);

    auto __DEFER_VAR_0 = flow::Defer([&](){println("Deferred 1");});    auto __DEFER_VAR_1 = flow::Defer([&](){println("Deferred 2");});
    msg = "middle";
    println(msg);

                flow::any a = make_any(42);
    if (flow::any_comprobate<int>(a)) {
        println("a is int");
    }

    flow::any b = str("hello");
    println(flow::any_cast<str>(b));

                int total = 0;
    for (int i = 1; i <= 5; i++) {
        total = total + i;
    }

    println(total);

    return 0;
}