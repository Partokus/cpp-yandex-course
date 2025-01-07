#include "Common.h"
#include <stdexcept>

using namespace std;

// Этот файл сдаётся на проверку
// Здесь напишите реализацию необходимых классов-потомков `IShape`
class Shape : public IShape
{
public:
    void SetPosition(Point point) final
    {
        _point = point;
    }

    Point GetPosition() const final
    {
        return _point;
    }

    void SetSize(Size size) final
    {
        _size = size;
    }

    Size GetSize() const final
    {
        return _size;
    }

    void SetTexture(shared_ptr<ITexture> texture) final
    {
        _texture = texture;
    }

    ITexture *GetTexture() const final
    {
        return _texture.get();
    }

    void Draw(Image &image) const override
    {
        for (int y = 0; y < _size.height; ++y)
        {
            for (int x = 0; x < _size.width; ++x)
            {
                if (IsEllipse() and not IsPointInEllipse(Point{x, y}, _size))
                {
                    continue;
                }

                Point point{x + _point.x, y + _point.y};

                // проверяем, что в пределах изображения
                if (point.y < image.size() and x < image[y].size())
                {
                    char symbol = '.';
                    // если пересечение с текстурой
                    if (_texture && y < _texture->GetSize().height && x < _texture->GetSize().width)
                    {
                        symbol = _texture->GetImage()[y][x];
                    }
                    image[point.y][point.x] = symbol;
                }
            }
        }
    }

    virtual bool IsEllipse() const
    {
        return false;
    }

protected:
    Point _point;
    Size _size;
    shared_ptr<ITexture> _texture;
};

class Rectangle : public Shape
{
public:
    std::unique_ptr<IShape> Clone() const override
    {
        return make_unique<Rectangle>(*this);
    }
};

class Ellipse : public Shape
{
public:
    std::unique_ptr<IShape> Clone() const override
    {
        return make_unique<Ellipse>(*this);
    }

    bool IsEllipse() const override
    {
        return true;
    }
};

unique_ptr<IShape> MakeShape(ShapeType shape_type)
{
    switch (shape_type)
    {
    case ShapeType::Rectangle:
        return make_unique<Rectangle>();
    case ShapeType::Ellipse:
        return make_unique<Ellipse>();
    }
    return make_unique<Rectangle>();
}