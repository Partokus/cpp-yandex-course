#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

class Function
{
public:
    void AddPart(const char &operation, double value)
    {
        function_parts.push_back({operation, value});
    }

    void Invert()
    {
        reverse(begin(function_parts), end(function_parts));
        for (auto &part : function_parts)
        {
            switch (part.operation)
            {
            case '+':
                part.operation = '-';
                break;
            case '-':
                part.operation = '+';
                break;
            case '*':
                part.operation = '/';
                break;
            case '/':
                part.operation = '*';
                break;
            }
        }
    }

    double Apply(double quality_or_weight) const
    {
        for (const auto &part : function_parts)
        {
            switch (part.operation)
            {
            case '+':
                quality_or_weight += part.value;
                break;
            case '-':
                quality_or_weight -= part.value;
                break;
            case '*':
                quality_or_weight *= part.value;
                break;
            case '/':
                quality_or_weight /= part.value;
                break;
            }
        }
        return quality_or_weight;
    }

private:
    struct FunctionPart
    {
        char operation;
        double value;
    };
    vector<FunctionPart> function_parts;
};

struct Image
{
    double quality;
    double freshness;
    double rating;
};

struct Params
{
    double a;
    double b;
    double c;
};

Function MakeWeightFunction(const Params &params, const Image &image)
{
    Function function;
    function.AddPart('*', params.a);
    function.AddPart('-', image.freshness * params.b);
    function.AddPart('+', image.rating * params.c);
    return function;
}

double ComputeImageWeight(const Params &params, const Image &image)
{
    Function function = MakeWeightFunction(params, image);
    return function.Apply(image.quality);
}

double ComputeQualityByWeight(const Params &params, const Image &image, double weight)
{
    Function function = MakeWeightFunction(params, image);
    function.Invert();
    return function.Apply(weight);
}

int main()
{
    Image image = {10, 2, 6};
    Params params = {4, 2, 6};
    cout << ComputeImageWeight(params, image) << endl;
    cout << ComputeQualityByWeight(params, image, 52) << endl;
    return 0;
}
