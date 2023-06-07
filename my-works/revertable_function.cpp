#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <optional>

using namespace std;

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

class Function
{
public:
    void AddPart(const char &operation, double value)
    {
        function_parts.push_back({operation, value});
    }

    void Invert()
    {
        for (auto &part : function_parts)
        {
            if (part.operation == '+')
            {
                part.operation = '-';
            }
            else
            {
                part.operation = '+';
            }
        }
    }

    double Apply(double quality_or_weight) const
    {
        for (const auto &part : function_parts)
        {
            if (part.operation == '+')
            {
                quality_or_weight += part.value;
            }
            else
            {
                quality_or_weight -= part.value;
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

Function MakeWeightFunction(const Params &params, const Image &image)
{
    Function function;
    function.AddPart('-', image.freshness * params.a + params.b);
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
    cout << ComputeQualityByWeight(params, image, 46) << endl;
    return 0;
}
