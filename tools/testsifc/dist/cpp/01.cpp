#include <flow/types>
#include <flow/io> 

using namespace flow;

flow::any make_any(int x) {
    return x;
}

int main() {
    flow::str msg = "start";
    println(msg);

    auto d1 = flow::Defer([&](){println("Deferred 1");});
    auto d2 = flow::Defer([&](){println("Deferred 2");});

    msg = "middle";
    println(msg);

                flow::any a = make_any(42);
    if (flow::any_comprobate<int>(a)) {
        println("a is int");
    }

    flow::any b = flow::str("hello");
    println(flow::any_cast<flow::str>(b));

                int total = 0;
    for (int i = 1; i <= 5; i++) {
        total = total + i;
    }

    println(total);

    return 0;
}