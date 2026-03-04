#include <flow/types>
#include <flow/result>

#include <flow/io>

#include <flow/random>

using namespace flow;
Result<int, int> funcionQuePuedeDarError(){
    RNG rng;
    if (rng.decimalRange(0.0, 1.0) < 0.5) return Result<int, int>::Err(1);
    return Result<int, int>::Ok(0);
}

bool trymain(){
    Result<int, int> res = funcionQuePuedeDarError();
    {auto __flow_tmp = res; if(!__flow_tmp) return true;};
    return false;
}

void trymain2(){
    Result<int, int> res = funcionQuePuedeDarError();
    {auto __flow_tmp = res; if(!__flow_tmp) return;};
}

int main(){
    trymain2();
    println(trymain());
    return 0;
}