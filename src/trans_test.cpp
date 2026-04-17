#include "trans_test.h"
#include "test_runner.h"
#include "profile.h"
#include <fstream>

using namespace std;
using namespace std::chrono;
using namespace std::chrono_literals;

bool AssertDouble(double lhs, double rhs)
{
    return lhs > (rhs - 0.1) and lhs < (rhs + 0.1);
}

void TestMakeRenderSettigs()
{
        using namespace Json;
    {
        istringstream input(R"({
    "render_settings": {
        "width": 1200,
        "height": 1200,
        "padding": 50,
        "stop_radius": 5,
        "line_width": 14,
        "stop_label_font_size": 20,
        "stop_label_offset": [
            7,
            -3
        ],
        "underlayer_color": [
            255,
            255,
            255,
            0.85
        ],
        "underlayer_width": 3,
        "color_palette": [
            "green",
            [
                255,
                160,
                0
            ],
            "red",
            [
                200,
                160,
                5,
                0.8
            ]
        ]
    }
}
)");
        Document doc = Load(input);

        const map<string, Node> &root = doc.GetRoot().AsMap();
        const map<string, Node> &render_settings_json = root.at("render_settings"s).AsMap();
        RenderSettings rs = MakeRenderSettigs(render_settings_json);

        RenderSettings expect{};
        expect.width = 1200;
        expect.height = 1200;
        expect.padding = 50;
        expect.stop_radius = 5;
        expect.line_width = 14;
        expect.stop_label_font_size = 20;
        expect.stop_label_offset = { 7, -3 };
        expect.underlayer_color = Svg::Rgba{255, 255, 255, 0.85};
        expect.underlayer_width = 3;
        expect.color_palette.push_back("green");
        expect.color_palette.push_back(Svg::Rgb{255, 160, 0});
        expect.color_palette.push_back("red");
        expect.color_palette.push_back(Svg::Rgba{200, 160, 5, 0.8});

        ASSERT(rs == expect);
    }
}

void TestCreateMap()
{
    using namespace Json;
    {
        istringstream input(R"({
    "routing_settings": {
        "bus_wait_time": 2,
        "bus_velocity": 30
    },
    "render_settings": {
        "width": 1200,
        "height": 1200,
        "padding": 50,
        "stop_radius": 5,
        "line_width": 14,
        "stop_label_font_size": 20,
        "stop_label_offset": [
            7,
            -3
        ],
        "underlayer_color": [
            255,
            255,
            255,
            0.85
        ],
        "underlayer_width": 3,
        "color_palette": [
            "green",
            [
                255,
                160,
                0
            ],
            "red"
        ]
    },
    "base_requests": [
        {
            "type": "Bus",
            "name": "14",
            "stops": [
                "Улица Лизы Чайкиной",
                "Электросети",
                "Ривьерский мост",
                "Гостиница Сочи",
                "Кубанская улица",
                "По требованию",
                "Улица Докучаева",
                "Улица Лизы Чайкиной"
            ],
            "is_roundtrip": true
        },
        {
            "type": "Bus",
            "name": "24",
            "stops": [
                "Улица Докучаева",
                "Параллельная улица",
                "Электросети",
                "Санаторий Родина"
            ],
            "is_roundtrip": false
        },
        {
            "type": "Bus",
            "name": "114",
            "stops": [
                "Морской вокзал",
                "Ривьерский мост"
            ],
            "is_roundtrip": false
        },
        {
            "type": "Stop",
            "name": "Улица Лизы Чайкиной",
            "latitude": 43.590317,
            "longitude": 39.746833,
            "road_distances": {
                "Электросети": 4300,
                "Улица Докучаева": 2000
            }
        },
        {
            "type": "Stop",
            "name": "Морской вокзал",
            "latitude": 43.581969,
            "longitude": 39.719848,
            "road_distances": {
                "Ривьерский мост": 850
            }
        },
        {
            "type": "Stop",
            "name": "Электросети",
            "latitude": 43.598701,
            "longitude": 39.730623,
            "road_distances": {
                "Санаторий Родина": 4500,
                "Параллельная улица": 1200,
                "Ривьерский мост": 1900
            }
        },
        {
            "type": "Stop",
            "name": "Ривьерский мост",
            "latitude": 43.587795,
            "longitude": 39.716901,
            "road_distances": {
                "Морской вокзал": 850,
                "Гостиница Сочи": 1740
            }
        },
        {
            "type": "Stop",
            "name": "Гостиница Сочи",
            "latitude": 43.578079,
            "longitude": 39.728068,
            "road_distances": {
                "Кубанская улица": 320
            }
        },
        {
            "type": "Stop",
            "name": "Кубанская улица",
            "latitude": 43.578509,
            "longitude": 39.730959,
            "road_distances": {
                "По требованию": 370
            }
        },
        {
            "type": "Stop",
            "name": "По требованию",
            "latitude": 43.579285,
            "longitude": 39.733742,
            "road_distances": {
                "Улица Докучаева": 600
            }
        },
        {
            "type": "Stop",
            "name": "Улица Докучаева",
            "latitude": 43.585586,
            "longitude": 39.733879,
            "road_distances": {
                "Параллельная улица": 1100
            }
        },
        {
            "type": "Stop",
            "name": "Параллельная улица",
            "latitude": 43.590041,
            "longitude": 39.732886,
            "road_distances": {}
        },
        {
            "type": "Stop",
            "name": "Санаторий Родина",
            "latitude": 43.601202,
            "longitude": 39.715498,
            "road_distances": {}
        }
    ],
    "stat_requests": [
        {
            "id": 826874078,
            "type": "Bus",
            "name": "14"
        },
        {
            "id": 1086967114,
            "type": "Route",
            "from": "Морской вокзал",
            "to": "Параллельная улица"
        },
        {
            "id": 1218663236,
            "type": "Map"
        }
    ]
}
)");
        ostringstream oss;
        DataBase db;
        Parse(input, oss, db);

        // istringstream expect(R"(<?xml version=\"1.0\" encoding=\"UTF-8\" ?><svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"><polyline points=\"202.705,725.165 99.2516,520.646 \" fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"1150,432.113 580.956,137.796 99.2516,520.646 491.264,861.722 592.751,846.627 690.447,819.386 695.256,598.192 1150,432.113 \" fill=\"none\" stroke=\"rgb(255,160,0)\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"695.256,598.192 660.397,441.801 580.956,137.796 50,50 \" fill=\"none\" stroke=\"red\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><circle cx=\"491.264\" cy=\"861.722\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"491.264\" y=\"861.722\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Гостиница Сочи</text><text x=\"491.264\" y=\"861.722\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Гостиница Сочи</text><circle cx=\"592.751\" cy=\"846.627\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"592.751\" y=\"846.627\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Кубанская улица</text><text x=\"592.751\" y=\"846.627\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Кубанская улица</text><circle cx=\"202.705\" cy=\"725.165\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"202.705\" y=\"725.165\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Морской вокзал</text><text x=\"202.705\" y=\"725.165\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Морской вокзал</text><circle cx=\"660.397\" cy=\"441.801\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"660.397\" y=\"441.801\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Параллельная улица</text><text x=\"660.397\" y=\"441.801\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Параллельная улица</text><circle cx=\"690.447\" cy=\"819.386\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"690.447\" y=\"819.386\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >По требованию</text><text x=\"690.447\" y=\"819.386\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >По требованию</text><circle cx=\"99.2516\" cy=\"520.646\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"99.2516\" y=\"520.646\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Ривьерский мост</text><text x=\"99.2516\" y=\"520.646\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Ривьерский мост</text><circle cx=\"50\" cy=\"50\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"50\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Родина</text><text x=\"50\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Родина</text><circle cx=\"695.256\" cy=\"598.192\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"695.256\" y=\"598.192\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Докучаева</text><text x=\"695.256\" y=\"598.192\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Докучаева</text><circle cx=\"1150\" cy=\"432.113\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"1150\" y=\"432.113\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Лизы Чайкиной</text><text x=\"1150\" y=\"432.113\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Лизы Чайкиной</text><circle cx=\"580.956\" cy=\"137.796\" r=\"5\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"580.956\" y=\"137.796\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Электросети</text><text x=\"580.956\" y=\"137.796\" dx=\"7\" dy=\"-3\" font-size=\"20\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Электросети</text></svg>)");

        // ASSERT_EQUAL(db.map, expect.str());
    }
    {
        istringstream input(R"({
    "stat_requests": [
        {
            "type": "Map",
            "id": 1821171961
        }
    ],
    "render_settings": {
        "underlayer_width": 3,
        "height": 950,
        "stop_radius": 3,
        "underlayer_color": [
            255,
            255,
            255,
            0.85
        ],
        "width": 1500,
        "stop_label_font_size": 13,
        "padding": 50,
        "color_palette": [
            "red",
            "green",
            "blue",
            "brown",
            "orange"
        ],
        "stop_label_offset": [
            7,
            -3
        ],
        "line_width": 10
    },
    "base_requests": [
        {
            "stops": [
                "Санаторий Салют",
                "Санаторная улица",
                "Пансионат Нева",
                "Санаторий Радуга",
                "Санаторий Родина",
                "Спортивная",
                "Парк Ривьера",
                "Морской вокзал",
                "Органный зал",
                "Театральная",
                "Пансионат Светлана",
                "Цирк",
                "Стадион",
                "Санаторий Металлург",
                "Улица Бытха"
            ],
            "name": "23",
            "type": "Bus",
            "is_roundtrip": false
        },
        {
            "stops": [
                "Улица Лизы Чайкиной",
                "Пионерская улица, 111",
                "Садовая",
                "Театральная"
            ],
            "name": "13",
            "type": "Bus",
            "is_roundtrip": false
        },
        {
            "stops": [
                "Морской вокзал",
                "Сбербанк",
                "Автовокзал",
                "Отель Звёздный",
                "Магазин Быт",
                "Хлебозавод",
                "Кинотеатр Юбилейный",
                "Новая Заря",
                "Деревообр. комбинат",
                "Целинная улица, 5",
                "Целинная улица, 57",
                "Целинная улица"
            ],
            "name": "36",
            "type": "Bus",
            "is_roundtrip": false
        },
        {
            "stops": [
                "Пансионат Светлана",
                "Улица Лысая Гора",
                "Улица В. Лысая Гора"
            ],
            "name": "44к",
            "type": "Bus",
            "is_roundtrip": false
        },
        {
            "stops": [
                "Краево-Греческая улица",
                "Улица Бытха",
                "Санаторий им. Ворошилова",
                "Санаторий Приморье",
                "Санаторий Заря",
                "Мацеста",
                "Мацестинская долина"
            ],
            "name": "90",
            "type": "Bus",
            "is_roundtrip": false
        },
        {
            "name": "Краево-Греческая улица",
            "latitude": 43.565551,
            "type": "Stop",
            "longitude": 39.776858,
            "road_distances": {
                "Улица Бытха": 1780
            }
        },
        {
            "name": "Санаторий им. Ворошилова",
            "latitude": 43.557935,
            "type": "Stop",
            "longitude": 39.764452,
            "road_distances": {
                "Санаторий Приморье": 950
            }
        },
        {
            "name": "Санаторий Приморье",
            "latitude": 43.554202,
            "type": "Stop",
            "longitude": 39.77256,
            "road_distances": {
                "Санаторий Заря": 2350
            }
        },
        {
            "name": "Санаторий Заря",
            "latitude": 43.549618,
            "type": "Stop",
            "longitude": 39.780908,
            "road_distances": {
                "Мацеста": 800
            }
        },
        {
            "name": "Мацеста",
            "latitude": 43.545509,
            "type": "Stop",
            "longitude": 39.788993,
            "road_distances": {
                "Мацестинская долина": 2350
            }
        },
        {
            "name": "Мацестинская долина",
            "latitude": 43.560422,
            "type": "Stop",
            "longitude": 39.798219,
            "road_distances": {}
        },
        {
            "name": "Улица Лысая Гора",
            "latitude": 43.577997,
            "type": "Stop",
            "longitude": 39.741685,
            "road_distances": {
                "Улица В. Лысая Гора": 640
            }
        },
        {
            "name": "Улица В. Лысая Гора",
            "latitude": 43.58092,
            "type": "Stop",
            "longitude": 39.744749,
            "road_distances": {}
        },
        {
            "name": "Морской вокзал",
            "latitude": 43.581969,
            "type": "Stop",
            "longitude": 39.719848,
            "road_distances": {
                "Сбербанк": 870,
                "Органный зал": 570
            }
        },
        {
            "name": "Сбербанк",
            "latitude": 43.585969,
            "type": "Stop",
            "longitude": 39.725175,
            "road_distances": {
                "Автовокзал": 870
            }
        },
        {
            "name": "Автовокзал",
            "latitude": 43.592956,
            "type": "Stop",
            "longitude": 39.727798,
            "road_distances": {
                "Отель Звёздный": 700
            }
        },
        {
            "name": "Отель Звёздный",
            "latitude": 43.596585,
            "type": "Stop",
            "longitude": 39.721151,
            "road_distances": {
                "Магазин Быт": 1000
            }
        },
        {
            "name": "Магазин Быт",
            "latitude": 43.604025,
            "type": "Stop",
            "longitude": 39.724492,
            "road_distances": {
                "Хлебозавод": 420
            }
        },
        {
            "name": "Хлебозавод",
            "latitude": 43.607364,
            "type": "Stop",
            "longitude": 39.726643,
            "road_distances": {
                "Кинотеатр Юбилейный": 2110
            }
        },
        {
            "name": "Кинотеатр Юбилейный",
            "latitude": 43.623382,
            "type": "Stop",
            "longitude": 39.720626,
            "road_distances": {
                "Новая Заря": 450
            }
        },
        {
            "name": "Новая Заря",
            "latitude": 43.626842,
            "type": "Stop",
            "longitude": 39.717802,
            "road_distances": {
                "Деревообр. комбинат": 530
            }
        },
        {
            "name": "Деревообр. комбинат",
            "latitude": 43.631035,
            "type": "Stop",
            "longitude": 39.714624,
            "road_distances": {
                "Целинная улица, 5": 840
            }
        },
        {
            "name": "Целинная улица, 5",
            "latitude": 43.633353,
            "type": "Stop",
            "longitude": 39.710257,
            "road_distances": {
                "Целинная улица, 57": 1270
            }
        },
        {
            "name": "Целинная улица, 57",
            "latitude": 43.640536,
            "type": "Stop",
            "longitude": 39.713253,
            "road_distances": {
                "Целинная улица": 1050
            }
        },
        {
            "name": "Целинная улица",
            "latitude": 43.647968,
            "type": "Stop",
            "longitude": 39.717733,
            "road_distances": {}
        },
        {
            "name": "Санаторий Салют",
            "latitude": 43.623238,
            "type": "Stop",
            "longitude": 39.704646,
            "road_distances": {
                "Санаторная улица": 1500
            }
        },
        {
            "name": "Санаторная улица",
            "latitude": 43.620766,
            "type": "Stop",
            "longitude": 39.719058,
            "road_distances": {
                "Пансионат Нева": 670
            }
        },
        {
            "name": "Пансионат Нева",
            "latitude": 43.614288,
            "type": "Stop",
            "longitude": 39.718674,
            "road_distances": {
                "Санаторий Радуга": 520
            }
        },
        {
            "name": "Санаторий Радуга",
            "latitude": 43.609951,
            "type": "Stop",
            "longitude": 39.72143,
            "road_distances": {
                "Санаторий Родина": 1190
            }
        },
        {
            "name": "Санаторий Родина",
            "latitude": 43.601202,
            "type": "Stop",
            "longitude": 39.715498,
            "road_distances": {
                "Спортивная": 1100
            }
        },
        {
            "name": "Спортивная",
            "latitude": 43.593689,
            "type": "Stop",
            "longitude": 39.717642,
            "road_distances": {
                "Парк Ривьера": 640
            }
        },
        {
            "name": "Парк Ривьера",
            "latitude": 43.588296,
            "type": "Stop",
            "longitude": 39.715956,
            "road_distances": {
                "Морской вокзал": 730
            }
        },
        {
            "name": "Органный зал",
            "latitude": 43.57926,
            "type": "Stop",
            "longitude": 39.725574,
            "road_distances": {
                "Театральная": 770
            }
        },
        {
            "name": "Пансионат Светлана",
            "latitude": 43.571807,
            "type": "Stop",
            "longitude": 39.735866,
            "road_distances": {
                "Цирк": 520,
                "Улица Лысая Гора": 1070
            }
        },
        {
            "name": "Цирк",
            "latitude": 43.569207,
            "type": "Stop",
            "longitude": 39.739869,
            "road_distances": {
                "Стадион": 860
            }
        },
        {
            "name": "Стадион",
            "latitude": 43.565301,
            "type": "Stop",
            "longitude": 39.749485,
            "road_distances": {
                "Санаторий Металлург": 950
            }
        },
        {
            "name": "Санаторий Металлург",
            "latitude": 43.561005,
            "type": "Stop",
            "longitude": 39.760511,
            "road_distances": {
                "Улица Бытха": 900
            }
        },
        {
            "name": "Улица Бытха",
            "latitude": 43.566135,
            "type": "Stop",
            "longitude": 39.762109,
            "road_distances": {
                "Санаторий им. Ворошилова": 1160
            }
        },
        {
            "name": "Улица Лизы Чайкиной",
            "latitude": 43.590317,
            "type": "Stop",
            "longitude": 39.746833,
            "road_distances": {
                "Пионерская улица, 111": 950
            }
        },
        {
            "name": "Пионерская улица, 111",
            "latitude": 43.587257,
            "type": "Stop",
            "longitude": 39.740325,
            "road_distances": {
                "Садовая": 520
            }
        },
        {
            "name": "Садовая",
            "latitude": 43.58395,
            "type": "Stop",
            "longitude": 39.736938,
            "road_distances": {
                "Театральная": 1300
            }
        },
        {
            "name": "Театральная",
            "latitude": 43.57471,
            "type": "Stop",
            "longitude": 39.731954,
            "road_distances": {
                "Пансионат Светлана": 390
            }
        }
    ],
    "routing_settings": {
        "bus_wait_time": 2,
        "bus_velocity": 30
    }
}
)");
        ostringstream oss;
        DataBase db;
        Parse(input, oss, db);

        // istringstream expect(R"(<?xml version=\"1.0\" encoding=\"UTF-8\" ?><svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"><polyline points=\"399.983,528.273 345.993,553.659 317.894,581.093 276.547,657.748 \" fill=\"none\" stroke=\"red\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"50,255.16 169.562,275.668 166.376,329.409 189.24,365.389 140.028,437.971 157.815,500.299 143.828,545.039 176.116,597.528 223.619,620.002 276.547,657.748 309.001,681.832 342.21,703.401 421.984,735.806 513.456,771.445 526.713,728.887 \" fill=\"none\" stroke=\"green\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"176.116,597.528 220.309,564.344 242.069,506.38 186.926,476.273 214.642,414.551 232.487,386.851 182.57,253.965 159.142,225.261 132.778,190.476 96.5489,171.246 121.404,111.656 158.57,50 \" fill=\"none\" stroke=\"blue\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"309.001,681.832 357.276,630.48 382.695,606.23 \" fill=\"none\" stroke=\"brown\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"649.071,733.732 526.713,728.887 546.151,796.914 613.415,827.883 682.67,865.912 749.743,900 826.282,776.282 \" fill=\"none\" stroke=\"orange\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><circle cx=\"242.069\" cy=\"506.38\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"242.069\" y=\"506.38\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Автовокзал</text><text x=\"242.069\" y=\"506.38\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Автовокзал</text><circle cx=\"132.778\" cy=\"190.476\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"132.778\" y=\"190.476\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Деревообр. комбинат</text><text x=\"132.778\" y=\"190.476\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Деревообр. комбинат</text><circle cx=\"182.57\" cy=\"253.965\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"182.57\" y=\"253.965\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Кинотеатр Юбилейный</text><text x=\"182.57\" y=\"253.965\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Кинотеатр Юбилейный</text><circle cx=\"649.071\" cy=\"733.732\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Краево-Греческая улица</text><text x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Краево-Греческая улица</text><circle cx=\"214.642\" cy=\"414.551\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"214.642\" y=\"414.551\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Магазин Быт</text><text x=\"214.642\" y=\"414.551\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Магазин Быт</text><circle cx=\"749.743\" cy=\"900\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"749.743\" y=\"900\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Мацеста</text><text x=\"749.743\" y=\"900\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Мацеста</text><circle cx=\"826.282\" cy=\"776.282\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Мацестинская долина</text><text x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Мацестинская долина</text><circle cx=\"176.116\" cy=\"597.528\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Морской вокзал</text><text x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Морской вокзал</text><circle cx=\"159.142\" cy=\"225.261\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"159.142\" y=\"225.261\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Новая Заря</text><text x=\"159.142\" y=\"225.261\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Новая Заря</text><circle cx=\"223.619\" cy=\"620.002\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"223.619\" y=\"620.002\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Органный зал</text><text x=\"223.619\" y=\"620.002\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Органный зал</text><circle cx=\"186.926\" cy=\"476.273\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"186.926\" y=\"476.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Отель Звёздный</text><text x=\"186.926\" y=\"476.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Отель Звёздный</text><circle cx=\"166.376\" cy=\"329.409\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"166.376\" y=\"329.409\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Пансионат Нева</text><text x=\"166.376\" y=\"329.409\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Пансионат Нева</text><circle cx=\"309.001\" cy=\"681.832\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Пансионат Светлана</text><text x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Пансионат Светлана</text><circle cx=\"143.828\" cy=\"545.039\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"143.828\" y=\"545.039\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Парк Ривьера</text><text x=\"143.828\" y=\"545.039\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Парк Ривьера</text><circle cx=\"345.993\" cy=\"553.659\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"345.993\" y=\"553.659\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Пионерская улица, 111</text><text x=\"345.993\" y=\"553.659\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Пионерская улица, 111</text><circle cx=\"317.894\" cy=\"581.093\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"317.894\" y=\"581.093\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Садовая</text><text x=\"317.894\" y=\"581.093\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Садовая</text><circle cx=\"682.67\" cy=\"865.912\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"682.67\" y=\"865.912\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Заря</text><text x=\"682.67\" y=\"865.912\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Заря</text><circle cx=\"513.456\" cy=\"771.445\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"513.456\" y=\"771.445\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Металлург</text><text x=\"513.456\" y=\"771.445\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Металлург</text><circle cx=\"613.415\" cy=\"827.883\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"613.415\" y=\"827.883\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Приморье</text><text x=\"613.415\" y=\"827.883\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Приморье</text><circle cx=\"189.24\" cy=\"365.389\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"189.24\" y=\"365.389\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Радуга</text><text x=\"189.24\" y=\"365.389\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Радуга</text><circle cx=\"140.028\" cy=\"437.971\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"140.028\" y=\"437.971\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Родина</text><text x=\"140.028\" y=\"437.971\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Родина</text><circle cx=\"50\" cy=\"255.16\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"50\" y=\"255.16\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Салют</text><text x=\"50\" y=\"255.16\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Салют</text><circle cx=\"546.151\" cy=\"796.914\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"546.151\" y=\"796.914\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий им. Ворошилова</text><text x=\"546.151\" y=\"796.914\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий им. Ворошилова</text><circle cx=\"169.562\" cy=\"275.668\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"169.562\" y=\"275.668\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторная улица</text><text x=\"169.562\" y=\"275.668\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторная улица</text><circle cx=\"220.309\" cy=\"564.344\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"220.309\" y=\"564.344\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Сбербанк</text><text x=\"220.309\" y=\"564.344\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Сбербанк</text><circle cx=\"157.815\" cy=\"500.299\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"157.815\" y=\"500.299\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Спортивная</text><text x=\"157.815\" y=\"500.299\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Спортивная</text><circle cx=\"421.984\" cy=\"735.806\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"421.984\" y=\"735.806\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Стадион</text><text x=\"421.984\" y=\"735.806\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Стадион</text><circle cx=\"276.547\" cy=\"657.748\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Театральная</text><text x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Театральная</text><circle cx=\"526.713\" cy=\"728.887\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Бытха</text><text x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Бытха</text><circle cx=\"382.695\" cy=\"606.23\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица В. Лысая Гора</text><text x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица В. Лысая Гора</text><circle cx=\"399.983\" cy=\"528.273\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Лизы Чайкиной</text><text x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Лизы Чайкиной</text><circle cx=\"357.276\" cy=\"630.48\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"357.276\" y=\"630.48\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Лысая Гора</text><text x=\"357.276\" y=\"630.48\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Лысая Гора</text><circle cx=\"232.487\" cy=\"386.851\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"232.487\" y=\"386.851\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Хлебозавод</text><text x=\"232.487\" y=\"386.851\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Хлебозавод</text><circle cx=\"158.57\" cy=\"50\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"158.57\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Целинная улица</text><text x=\"158.57\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Целинная улица</text><circle cx=\"96.5489\" cy=\"171.246\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"96.5489\" y=\"171.246\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Целинная улица, 5</text><text x=\"96.5489\" y=\"171.246\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Целинная улица, 5</text><circle cx=\"121.404\" cy=\"111.656\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"121.404\" y=\"111.656\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Целинная улица, 57</text><text x=\"121.404\" y=\"111.656\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Целинная улица, 57</text><circle cx=\"342.21\" cy=\"703.401\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"342.21\" y=\"703.401\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Цирк</text><text x=\"342.21\" y=\"703.401\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Цирк</text></svg>)");

        // ASSERT_EQUAL(db.map, expect.str());
    }
    {
        istringstream input(R"({
    "stat_requests": [
        {
            "type": "Map",
            "id": 1821171961
        },
        {
            "type": "Map",
            "id": 1821171211
        }
    ],
    "routing_settings": {"bus_wait_time": 2, "bus_velocity": 30},
    "render_settings": {"width": 1200, "height": 1200, "padding": 50, "stop_radius": 5, "line_width": 14, "stop_label_font_size": 20, "stop_label_offset": [7, -3], "underlayer_color": [255, 255, 255, 0.85], "underlayer_width": 3, "color_palette": ["green", [255, 160, 0], "red"]},
    "base_requests": [{"type": "Bus", "name": "14", "stops": ["Ulitsa Lizy Chaykinoy", "Elektroseti", "Riv'erskiy most", "Gostinitsa Sochi", "Kubanskaya ulitsa", "Po trebovaniyu", "Ulitsa Dokuchaeva", "Ulitsa Lizy Chaykinoy"], "is_roundtrip": true}, {"type": "Bus", "name": "24", "stops": ["Ulitsa Dokuchaeva", "Parallel'naya ulitsa", "Elektroseti", "Sanatoriy Rodina"], "is_roundtrip": false}, {"type": "Bus", "name": "114", "stops": ["Morskoy vokzal", "Riv'erskiy most"], "is_roundtrip": false}, {"type": "Stop", "name": "Ulitsa Lizy Chaykinoy", "latitude": 43.590317, "longitude": 39.746833, "road_distances": {"Elektroseti": 4300, "Ulitsa Dokuchaeva": 2000}}, {"type": "Stop", "name": "Morskoy vokzal", "latitude": 43.581969, "longitude": 39.719848, "road_distances": {"Riv'erskiy most": 850}}, {"type": "Stop", "name": "Elektroseti", "latitude": 43.598701, "longitude": 39.730623, "road_distances": {"Sanatoriy Rodina": 4500, "Parallel'naya ulitsa": 1200, "Riv'erskiy most": 1900}}, {"type": "Stop", "name": "Riv'erskiy most", "latitude": 43.587795, "longitude": 39.716901, "road_distances": {"Morskoy vokzal": 850, "Gostinitsa Sochi": 1740}}, {"type": "Stop", "name": "Gostinitsa Sochi", "latitude": 43.578079, "longitude": 39.728068, "road_distances": {"Kubanskaya ulitsa": 320}}, {"type": "Stop", "name": "Kubanskaya ulitsa", "latitude": 43.578509, "longitude": 39.730959, "road_distances": {"Po trebovaniyu": 370}}, {"type": "Stop", "name": "Po trebovaniyu", "latitude": 43.579285, "longitude": 39.733742, "road_distances": {"Ulitsa Dokuchaeva": 600}}, {"type": "Stop", "name": "Ulitsa Dokuchaeva", "latitude": 43.585586, "longitude": 39.733879, "road_distances": {"Parallel'naya ulitsa": 1100}}, {"type": "Stop", "name": "Parallel'naya ulitsa", "latitude": 43.590041, "longitude": 39.732886, "road_distances": {}}, {"type": "Stop", "name": "Sanatoriy Rodina", "latitude": 43.601202, "longitude": 39.715498, "road_distances": {}
}
)");
        ostringstream oss;
        DataBase db;
        Parse(input, oss, db);

        // cout << oss.str() << endl;

        // istringstream expect(R"(<?xml version=\"1.0\" encoding=\"UTF-8\" ?><svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"><polyline points=\"399.983,528.273 345.993,553.659 317.894,581.093 276.547,657.748 \" fill=\"none\" stroke=\"red\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"50,255.16 169.562,275.668 166.376,329.409 189.24,365.389 140.028,437.971 157.815,500.299 143.828,545.039 176.116,597.528 223.619,620.002 276.547,657.748 309.001,681.832 342.21,703.401 421.984,735.806 513.456,771.445 526.713,728.887 \" fill=\"none\" stroke=\"green\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"176.116,597.528 220.309,564.344 242.069,506.38 186.926,476.273 214.642,414.551 232.487,386.851 182.57,253.965 159.142,225.261 132.778,190.476 96.5489,171.246 121.404,111.656 158.57,50 \" fill=\"none\" stroke=\"blue\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"309.001,681.832 357.276,630.48 382.695,606.23 \" fill=\"none\" stroke=\"brown\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><polyline points=\"649.071,733.732 526.713,728.887 546.151,796.914 613.415,827.883 682.67,865.912 749.743,900 826.282,776.282 \" fill=\"none\" stroke=\"orange\" stroke-width=\"10\" stroke-linecap=\"round\" stroke-linejoin=\"round\" /><circle cx=\"242.069\" cy=\"506.38\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"242.069\" y=\"506.38\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Автовокзал</text><text x=\"242.069\" y=\"506.38\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Автовокзал</text><circle cx=\"132.778\" cy=\"190.476\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"132.778\" y=\"190.476\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Деревообр. комбинат</text><text x=\"132.778\" y=\"190.476\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Деревообр. комбинат</text><circle cx=\"182.57\" cy=\"253.965\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"182.57\" y=\"253.965\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Кинотеатр Юбилейный</text><text x=\"182.57\" y=\"253.965\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Кинотеатр Юбилейный</text><circle cx=\"649.071\" cy=\"733.732\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Краево-Греческая улица</text><text x=\"649.071\" y=\"733.732\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Краево-Греческая улица</text><circle cx=\"214.642\" cy=\"414.551\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"214.642\" y=\"414.551\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Магазин Быт</text><text x=\"214.642\" y=\"414.551\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Магазин Быт</text><circle cx=\"749.743\" cy=\"900\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"749.743\" y=\"900\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Мацеста</text><text x=\"749.743\" y=\"900\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Мацеста</text><circle cx=\"826.282\" cy=\"776.282\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Мацестинская долина</text><text x=\"826.282\" y=\"776.282\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Мацестинская долина</text><circle cx=\"176.116\" cy=\"597.528\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Морской вокзал</text><text x=\"176.116\" y=\"597.528\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Морской вокзал</text><circle cx=\"159.142\" cy=\"225.261\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"159.142\" y=\"225.261\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Новая Заря</text><text x=\"159.142\" y=\"225.261\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Новая Заря</text><circle cx=\"223.619\" cy=\"620.002\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"223.619\" y=\"620.002\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Органный зал</text><text x=\"223.619\" y=\"620.002\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Органный зал</text><circle cx=\"186.926\" cy=\"476.273\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"186.926\" y=\"476.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Отель Звёздный</text><text x=\"186.926\" y=\"476.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Отель Звёздный</text><circle cx=\"166.376\" cy=\"329.409\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"166.376\" y=\"329.409\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Пансионат Нева</text><text x=\"166.376\" y=\"329.409\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Пансионат Нева</text><circle cx=\"309.001\" cy=\"681.832\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Пансионат Светлана</text><text x=\"309.001\" y=\"681.832\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Пансионат Светлана</text><circle cx=\"143.828\" cy=\"545.039\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"143.828\" y=\"545.039\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Парк Ривьера</text><text x=\"143.828\" y=\"545.039\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Парк Ривьера</text><circle cx=\"345.993\" cy=\"553.659\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"345.993\" y=\"553.659\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Пионерская улица, 111</text><text x=\"345.993\" y=\"553.659\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Пионерская улица, 111</text><circle cx=\"317.894\" cy=\"581.093\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"317.894\" y=\"581.093\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Садовая</text><text x=\"317.894\" y=\"581.093\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Садовая</text><circle cx=\"682.67\" cy=\"865.912\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"682.67\" y=\"865.912\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Заря</text><text x=\"682.67\" y=\"865.912\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Заря</text><circle cx=\"513.456\" cy=\"771.445\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"513.456\" y=\"771.445\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Металлург</text><text x=\"513.456\" y=\"771.445\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Металлург</text><circle cx=\"613.415\" cy=\"827.883\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"613.415\" y=\"827.883\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Приморье</text><text x=\"613.415\" y=\"827.883\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Приморье</text><circle cx=\"189.24\" cy=\"365.389\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"189.24\" y=\"365.389\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Радуга</text><text x=\"189.24\" y=\"365.389\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Радуга</text><circle cx=\"140.028\" cy=\"437.971\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"140.028\" y=\"437.971\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Родина</text><text x=\"140.028\" y=\"437.971\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Родина</text><circle cx=\"50\" cy=\"255.16\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"50\" y=\"255.16\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий Салют</text><text x=\"50\" y=\"255.16\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий Салют</text><circle cx=\"546.151\" cy=\"796.914\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"546.151\" y=\"796.914\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторий им. Ворошилова</text><text x=\"546.151\" y=\"796.914\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторий им. Ворошилова</text><circle cx=\"169.562\" cy=\"275.668\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"169.562\" y=\"275.668\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Санаторная улица</text><text x=\"169.562\" y=\"275.668\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Санаторная улица</text><circle cx=\"220.309\" cy=\"564.344\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"220.309\" y=\"564.344\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Сбербанк</text><text x=\"220.309\" y=\"564.344\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Сбербанк</text><circle cx=\"157.815\" cy=\"500.299\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"157.815\" y=\"500.299\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Спортивная</text><text x=\"157.815\" y=\"500.299\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Спортивная</text><circle cx=\"421.984\" cy=\"735.806\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"421.984\" y=\"735.806\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Стадион</text><text x=\"421.984\" y=\"735.806\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Стадион</text><circle cx=\"276.547\" cy=\"657.748\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Театральная</text><text x=\"276.547\" y=\"657.748\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Театральная</text><circle cx=\"526.713\" cy=\"728.887\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Бытха</text><text x=\"526.713\" y=\"728.887\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Бытха</text><circle cx=\"382.695\" cy=\"606.23\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица В. Лысая Гора</text><text x=\"382.695\" y=\"606.23\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица В. Лысая Гора</text><circle cx=\"399.983\" cy=\"528.273\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Лизы Чайкиной</text><text x=\"399.983\" y=\"528.273\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Лизы Чайкиной</text><circle cx=\"357.276\" cy=\"630.48\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"357.276\" y=\"630.48\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Улица Лысая Гора</text><text x=\"357.276\" y=\"630.48\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Улица Лысая Гора</text><circle cx=\"232.487\" cy=\"386.851\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"232.487\" y=\"386.851\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Хлебозавод</text><text x=\"232.487\" y=\"386.851\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Хлебозавод</text><circle cx=\"158.57\" cy=\"50\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"158.57\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Целинная улица</text><text x=\"158.57\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Целинная улица</text><circle cx=\"96.5489\" cy=\"171.246\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"96.5489\" y=\"171.246\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Целинная улица, 5</text><text x=\"96.5489\" y=\"171.246\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Целинная улица, 5</text><circle cx=\"121.404\" cy=\"111.656\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"121.404\" y=\"111.656\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Целинная улица, 57</text><text x=\"121.404\" y=\"111.656\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Целинная улица, 57</text><circle cx=\"342.21\" cy=\"703.401\" r=\"3\" fill=\"white\" stroke=\"none\" stroke-width=\"1\" /><text x=\"342.21\" y=\"703.401\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"rgba(255,255,255,0.850000)\" stroke=\"rgba(255,255,255,0.850000)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" >Цирк</text><text x=\"342.21\" y=\"703.401\" dx=\"7\" dy=\"-3\" font-size=\"13\" font-family=\"Verdana\" fill=\"black\" stroke=\"none\" stroke-width=\"1\" >Цирк</text></svg>)");

        // ASSERT_EQUAL(db.map, expect.str());
    }
}

void TestParseAddStopQuery()
{
    using namespace Json;
    {
        istringstream input(R"({
    "type": "Stop",
    "road_distances": {},
    "longitude": 2.34,
    "name": "Empire   Street Building 5",
    "latitude": 55.611087
}
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop = ParseAddStopQuery(doc.GetRoot().AsMap(), db);
        Stop expect{
            .name = "Empire   Street Building 5",
            .latitude = 55.611087,
            .longitude = 2.34
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
        ASSERT_EQUAL(db.road_route_length.size(), 0U);
    }
    {
        istringstream input(R"({
    "type": "Stop",
    "road_distances": {},
    "longitude": 89.5,
    "name": "E",
    "latitude": 21.0
}
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop = ParseAddStopQuery(doc.GetRoot().AsMap(), db);
        Stop expect{
            .name = "E",
            .latitude = 21.0,
            .longitude = 89.5
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
        ASSERT_EQUAL(db.road_route_length.size(), 0U);
    }

    // with Dim to stop#
    {
        istringstream input(R"({
    "type": "Stop",
    "road_distances": {
        "Marushkino": 3900
    },
    "longitude": 89.591,
    "name": "Univer",
    "latitude": 21.666
}
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop = ParseAddStopQuery(doc.GetRoot().AsMap(), db);
        Stop expect{
            .name = "Univer",
            .latitude = 21.666,
            .longitude = 89.591
        };
        ASSERT_EQUAL(stop->name, expect.name);
        ASSERT_EQUAL(stop->latitude, expect.latitude);
        ASSERT_EQUAL(stop->longitude, expect.longitude);
        auto it = db.road_route_length.find(stop);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[make_shared<Stop>(Stop{"Marushkino"})], 3900U);
    }
    {
        istringstream input(R"([
    {
        "type": "Stop",
        "road_distances": {
            "Marushkino": 3900,
            "Sabotajnaya": 1000,
            "Meteornaya": 100000
        },
        "longitude": 89.591,
        "name": "Univer",
        "latitude": 21.666
    },
    {
        "type": "Stop",
        "road_distances": {
            "Univer": 500
        },
        "longitude": 89.2,
        "name": "Meteornaya",
        "latitude": 21.1
    }
]
)");
        Document doc = Load(input);
        DataBase db;
        StopPtr stop_univer = ParseAddStopQuery(doc.GetRoot().AsArray().at(0).AsMap(), db);
        StopPtr stop_meteornaya = ParseAddStopQuery(doc.GetRoot().AsArray().at(1).AsMap(), db);
        Stop expect{
            .name = "Univer",
            .latitude = 21.666,
            .longitude = 89.591
        };
        StopPtr stop_marushkino = make_shared<Stop>(Stop{"Marushkino"});
        StopPtr stop_sabotajnaya = make_shared<Stop>(Stop{"Sabotajnaya"});
        ASSERT_EQUAL(stop_univer->name, expect.name);
        ASSERT_EQUAL(stop_univer->latitude, expect.latitude);
        ASSERT_EQUAL(stop_univer->longitude, expect.longitude);
        expect.name = "Meteornaya";
        expect.latitude = 21.1;
        expect.longitude = 89.2;
        ASSERT_EQUAL(stop_meteornaya->name, expect.name);
        ASSERT_EQUAL(stop_meteornaya->latitude, expect.latitude);
        ASSERT_EQUAL(stop_meteornaya->longitude, expect.longitude);
        auto it = db.road_route_length.find(stop_univer);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 3U);
        ASSERT_EQUAL(it->second[stop_marushkino], 3900U);
        ASSERT_EQUAL(it->second[stop_sabotajnaya], 1000U);
        ASSERT_EQUAL(it->second[stop_meteornaya], 100000U);

        it = db.road_route_length.find(stop_marushkino);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[stop_univer], 3900U);

        it = db.road_route_length.find(stop_sabotajnaya);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[stop_univer], 1000U);

        it = db.road_route_length.find(stop_meteornaya);
        ASSERT(it != db.road_route_length.end());
        ASSERT_EQUAL(it->second.size(), 1U);
        ASSERT_EQUAL(it->second[stop_univer], 500U);
    }
}

void TestParseAddBusQuery()
{
    using namespace Json;
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "256",
    "stops": [
        "Biryulyovo  Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo  Tovarnaya"
    ],
    "is_roundtrip": false
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo  Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo  Tovarnaya"});
        Stops stops{stop1, stop2, stop3, stop4};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4},
            .ring = false
        };
        ASSERT_EQUAL(bus->name, expect.name);
        for (int i = 0; i < 4; ++i)
        {
            ASSERT(bus->stops[i].get() == expect.stops[i].get());
            ASSERT(*bus->stops[i] == *expect.stops[i]);
        }
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "256",
    "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
    ],
    "is_roundtrip": true
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya"});
        Stops stops{stop1, stop2, stop3, stop4, stop5};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "256",
            .stops = {stop1, stop2, stop3, stop4, stop5, stop1},
            .ring = true
        };
        ASSERT_EQUAL(bus->stops.size(), expect.stops.size());
        ASSERT_EQUAL(bus->name, expect.name);
        for (int i = 0; i < 5; ++i)
            ASSERT(bus->stops[i].get() == expect.stops[i].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "My Bus",
    "stops": [
        "Biryulyovo Zapadnoye",
        "Tomash"
    ],
    "is_roundtrip": false
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Tomash"});
        Stops stops{stop1, stop2};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "My Bus",
            .stops = {stop1, stop2},
            .ring = false
        };
        ASSERT_EQUAL(bus->name, expect.name);
        ASSERT(bus->stops[0].get() == expect.stops[0].get());
        ASSERT(bus->stops[1].get() == expect.stops[1].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
    {
        istringstream input(R"({
    "type": "Bus",
    "name": "My Bus  HardBass",
    "stops": [
        "Biryulyovo",
        "Tomash"
    ],
    "is_roundtrip": false
}
)");
        Document doc = Load(input);
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Tomash"});
        Stops stops{stop1, stop2};
        BusPtr bus = ParseAddBusQuery(doc.GetRoot().AsMap(), stops);
        Bus expect{
            .name = "My Bus  HardBass",
            .stops = {stop1, stop2},
            .ring = false
        };
        ASSERT_EQUAL(bus->name, expect.name);
        ASSERT(bus->stops[0].get() == expect.stops[0].get());
        ASSERT(bus->stops[1].get() == expect.stops[1].get());
        ASSERT_EQUAL(bus->ring, expect.ring);
    }
}

void TestCalcGeoDistance()
{
    {
        double lat1 = 55.611087;
        double lon1 = 37.20829;
        double lat2 = 55.595884;
        double lon2 = 37.209755;

        double length = CalcGeoDistance(lat1, lon1, lat2, lon2);
        ASSERT(length > 1692.0 and length < 1694.0);
        length = CalcGeoDistance(lat2, lon2, lat1, lon1);
        ASSERT(length > 1692.0 and length < 1694.0);
        length = CalcGeoDistance(lat1, lat2, lat1, lat2);
        ASSERT_EQUAL(length, 0.0);
    }
}

void TestDataBaseCreateInfo()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya"});
        StopPtr stop6 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop7 = make_shared<Stop>(Stop{"Lepeshkina"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5, stop6}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop6, stop5, stop4, stop3, stop7, stop6}});
        bus3->ring = true;

        // уникальные остановки: Biryulyovo Zapadnoye, Biryusinka, Lepeshkina

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7};
        db.buses = {bus1, bus2, bus3};

        db.CreateInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 5U);
        ASSERT_EQUAL(db.buses_info[bus2].stops_on_route, 7U);
        ASSERT_EQUAL(db.buses_info[bus3].stops_on_route, 6U);

        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 3U);
        ASSERT_EQUAL(db.buses_info[bus2].unique_stops, 4U);
        ASSERT_EQUAL(db.buses_info[bus3].unique_stops, 5U);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Tolstopaltsevo", 55.611087, 37.20829});
        StopPtr stop2 = make_shared<Stop>(Stop{"Marushkino", 55.595884, 37.209755});
        StopPtr stop3 = make_shared<Stop>(Stop{"Rasskazovka", 55.632761, 37.333324});
        StopPtr stop4 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye", 55.574371, 37.6517});
        StopPtr stop5 = make_shared<Stop>(Stop{"Biryusinka", 55.581065, 37.64839});
        StopPtr stop6 = make_shared<Stop>(Stop{"Universam", 55.587655, 37.645687});
        StopPtr stop7 = make_shared<Stop>(Stop{"Biryulyovo Tovarnaya", 55.592028, 37.653656});
        StopPtr stop8 = make_shared<Stop>(Stop{"Biryulyovo Passazhirskaya", 55.580999, 37.659164});

        BusPtr bus1 = make_shared<Bus>(Bus{"256", {stop4, stop5, stop6, stop7, stop8, stop4}});
        bus1->ring = true;
        BusPtr bus2 = make_shared<Bus>(Bus{"750", {stop1, stop2, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2};

        db.CreateInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 6U);
        ASSERT_EQUAL(db.buses_info[bus2].stops_on_route, 5U);

        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 5U);
        ASSERT_EQUAL(db.buses_info[bus2].unique_stops, 3U);

        ASSERT(db.buses_info[bus1].route_length_geo > 4370.0 and db.buses_info[bus1].route_length_geo < 4372.0);
        ASSERT(db.buses_info[bus2].route_length_geo > 20938.0 and db.buses_info[bus2].route_length_geo < 20940.0);
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Tolstopaltsevo", 55.611087, 37.20829});
        StopPtr stop2 = make_shared<Stop>(Stop{"Marushkino", 55.595884, 37.209755});
        StopPtr stop3 = make_shared<Stop>(Stop{"Rasskazovka", 55.632761, 37.333324});

        BusPtr bus1 = make_shared<Bus>(Bus{"256", {stop1, stop2, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3};
        db.buses = {bus1};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 1500;
        db.road_route_length[stop3][stop2] = 1500;

        db.CreateInfo();

        ASSERT_EQUAL(db.buses_info[bus1].stops_on_route, 5U);
        ASSERT_EQUAL(db.buses_info[bus1].unique_stops, 3U);
        ASSERT(db.buses_info[bus1].route_length_geo > 20938.0 and db.buses_info[bus1].route_length_geo < 20940.0);
    }
}

void TestDataBaseCreateGraph()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;

        db.CreateInfo(6, 40.0);

        ASSERT_EQUAL(db.routing_settings.bus_wait_time, 6U);
        ASSERT_EQUAL(db.routing_settings.bus_velocity, 40.0);
        ASSERT(AssertDouble(db.routing_settings.bus_velocity_meters_min, 666.6));
        ASSERT(AssertDouble(db.routing_settings.meters_past_while_wait_bus, 4000.0));
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop2, stop3, stop4, stop5}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;

        db.CreateInfo();
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop4, stop5}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;

        db.CreateInfo();
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5, stop3}});
        bus2->ring = true;

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5};
        db.buses = {bus1, bus2};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop5][stop3] = 400;

        db.CreateInfo();
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});
        StopPtr stop6 = make_shared<Stop>(Stop{"Ejevikovka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop3, stop6}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6};
        db.buses = {bus1, bus2, bus3};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop3][stop6] = 200;
        db.road_route_length[stop6][stop3] = 200;

        db.CreateInfo();
    }
}

void TestBuildRoute()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"Biryulyovo Zapadnoye"});
        StopPtr stop2 = make_shared<Stop>(Stop{"Biryusinka"});
        StopPtr stop3 = make_shared<Stop>(Stop{"Universam"});
        StopPtr stop4 = make_shared<Stop>(Stop{"Nekrasovka"});
        StopPtr stop5 = make_shared<Stop>(Stop{"Malinovka"});
        StopPtr stop6 = make_shared<Stop>(Stop{"Ejevikovka"});
        StopPtr stop7 = make_shared<Stop>(Stop{"Apelsinovka"});
        StopPtr stop8 = make_shared<Stop>(Stop{"Kivivka"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop3, stop6}});
        BusPtr bus4 = make_shared<Bus>(Bus{"844", {stop7, stop8}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2, bus3, bus4};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop3][stop6] = 200;
        db.road_route_length[stop6][stop3] = 200;
        db.road_route_length[stop7][stop8] = 100;
        db.road_route_length[stop8][stop7] = 200;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        Router router{db.graph};

        Graph::VertexId stop1_bus1_vertex_id = db.route_unit_to_vertex_id[stop1][bus1][stop2];
        Graph::VertexId stop2_bus1_vertex_id = db.route_unit_to_vertex_id[stop2][bus1][stop3];
        Graph::VertexId stop3_bus1_vertex_id = db.route_unit_to_vertex_id[stop3][bus1][nullptr];

        Graph::VertexId stop3_bus2_vertex_id = db.route_unit_to_vertex_id[stop3][bus2][stop4];
        Graph::VertexId stop4_bus2_vertex_id = db.route_unit_to_vertex_id[stop4][bus2][stop5];
        Graph::VertexId stop5_bus2_vertex_id = db.route_unit_to_vertex_id[stop5][bus2][nullptr];

        Graph::VertexId stop3_bus3_vertex_id = db.route_unit_to_vertex_id[stop3][bus3][stop6];
        Graph::VertexId stop6_bus3_vertex_id = db.route_unit_to_vertex_id[stop6][bus3][nullptr];

        Graph::VertexId stop7_bus4_vertex_id = db.route_unit_to_vertex_id[stop7][bus4][stop8];
        Graph::VertexId stop8_bus4_vertex_id = db.route_unit_to_vertex_id[stop8][bus4][nullptr];

        RouteInfo route_info = router.BuildRoute(stop1_bus1_vertex_id, stop3_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 1500.0));
        ASSERT_EQUAL(route_info.edge_count, 2U);
        Graph::EdgeId edge_id = router.GetRouteEdge(route_info.id, 0);
        Edge edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop1_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop3_bus1_vertex_id, stop1_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 1500.0));
        ASSERT_EQUAL(route_info.edge_count, 2U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop1_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop3_bus1_vertex_id, stop3_bus2_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 4000.0));
        router.ReleaseRoute(route_info.id);
        route_info = router.BuildRoute(stop3_bus1_vertex_id, stop3_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 4000.0));
        router.ReleaseRoute(route_info.id);
        route_info = router.BuildRoute(stop3_bus2_vertex_id, stop3_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 4000.0));
        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop1_bus1_vertex_id, stop5_bus2_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 6600.0));
        ASSERT_EQUAL(route_info.edge_count, 6U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop1_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, db.abstract_stop_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.abstract_stop_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 5);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop5_bus2_vertex_id);

        route_info = router.BuildRoute(stop5_bus2_vertex_id, stop1_bus1_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 6600.0));
        ASSERT_EQUAL(route_info.edge_count, 6U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop5_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, db.abstract_stop_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.abstract_stop_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 5);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop1_bus1_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop1_bus1_vertex_id, stop6_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 5700.0));
        ASSERT_EQUAL(route_info.edge_count, 5U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop1_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop2_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);

        ASSERT_EQUAL(edge.from, stop2_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus1_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus1_vertex_id);
        ASSERT_EQUAL(edge.to, db.abstract_stop_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.abstract_stop_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus3_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus3_vertex_id);
        ASSERT_EQUAL(edge.to, stop6_bus3_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop5_bus2_vertex_id, stop6_bus3_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 5300.0));
        ASSERT_EQUAL(route_info.edge_count, 5U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop5_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop4_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 1);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop4_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, stop3_bus2_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 2);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus2_vertex_id);
        ASSERT_EQUAL(edge.to, db.abstract_stop_to_vertex_id[stop3]);
        edge_id = router.GetRouteEdge(route_info.id, 3);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, db.abstract_stop_to_vertex_id[stop3]);
        ASSERT_EQUAL(edge.to, stop3_bus3_vertex_id);
        edge_id = router.GetRouteEdge(route_info.id, 4);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop3_bus3_vertex_id);
        ASSERT_EQUAL(edge.to, stop6_bus3_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop7_bus4_vertex_id, stop8_bus4_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 100.0));
        ASSERT_EQUAL(route_info.edge_count, 1U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop7_bus4_vertex_id);
        ASSERT_EQUAL(edge.to, stop8_bus4_vertex_id);

        router.ReleaseRoute(route_info.id);

        route_info = router.BuildRoute(stop8_bus4_vertex_id, stop7_bus4_vertex_id).value();
        ASSERT(AssertDouble(route_info.weight, 200.0));
        ASSERT_EQUAL(route_info.edge_count, 1U);
        edge_id = router.GetRouteEdge(route_info.id, 0);
        edge = db.graph.GetEdge(edge_id);
        ASSERT_EQUAL(edge.from, stop8_bus4_vertex_id);
        ASSERT_EQUAL(edge.to, stop7_bus4_vertex_id);

        router.ReleaseRoute(route_info.id);

        ASSERT(not router.BuildRoute(stop7_bus4_vertex_id, stop6_bus3_vertex_id));
        ASSERT(not router.BuildRoute(stop8_bus4_vertex_id, stop6_bus3_vertex_id));
        ASSERT(not router.BuildRoute(stop8_bus4_vertex_id, stop1_bus1_vertex_id));
        ASSERT(not router.BuildRoute(stop8_bus4_vertex_id, stop3_bus2_vertex_id));
        ASSERT(not router.BuildRoute(stop7_bus4_vertex_id, stop5_bus2_vertex_id));
    }
}

void TestParseJson()
{
    istringstream input(R"({
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {
        "Marushkino": 3900
      },
      "longitude": 37.20829,
      "name": "Tolstopaltsevo",
      "latitude": 55.611087
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 9900
      },
      "longitude": 37.209755,
      "name": "Marushkino",
      "latitude": 55.595884
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.333324,
      "name": "Rasskazovka",
      "latitude": 55.632761
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 7500,
        "Biryusinka": 1800,
        "Universam": 2400
      },
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye",
      "latitude": 55.574371
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 750
      },
      "longitude": 37.64839,
      "name": "Biryusinka",
      "latitude": 55.581065
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 5600,
        "Biryulyovo Tovarnaya": 900
      },
      "longitude": 37.645687,
      "name": "Universam",
      "latitude": 55.587655
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Passazhirskaya": 1300
      },
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya",
      "latitude": 55.592028
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Zapadnoye": 1200
      },
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya",
      "latitude": 55.580999
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Universam",
        "Rossoshanskaya ulitsa",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa",
      "latitude": 55.595579
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.603831,
      "name": "Prazhskaya",
      "latitude": 55.611678
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 1965312327
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 519139350
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 194217464
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 746888088
    },
    {
      "type": "Stop",
      "name": "Prazhskaya",
      "id": 65100610
    },
    {
      "type": "Stop",
      "name": "Biryulyovo Zapadnoye",
      "id": 1042838872
    }
  ]
}
)");
    using namespace Json;
    Document doc = Load(input);
    const map<string, Node> &root = doc.GetRoot().AsMap();
    ASSERT_EQUAL(root.size(), 2U);

    const vector<Node> &base_requests = root.at("base_requests"s).AsArray();
    ASSERT_EQUAL(base_requests.size(), 13U);

    size_t node_num = 0U;
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Marushkino": 3900
            },
            "longitude": 37.20829,
            "name": "Tolstopaltsevo",
            "latitude": 55.611087
        },
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Marushkino"s).AsInt(), 3900 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.20829) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.611087) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Tolstopaltsevo"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Rasskazovka": 9900
            },
            "longitude": 37.209755,
            "name": "Marushkino",
            "latitude": 55.595884
        },
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Rasskazovka"s).AsInt(), 9900 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.209755) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.595884) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Marushkino"s );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "256",
            "stops": [
                "Biryulyovo Zapadnoye",
                "Biryusinka",
                "Universam",
                "Biryulyovo Tovarnaya",
                "Biryulyovo Passazhirskaya",
                "Biryulyovo Zapadnoye"
            ],
            "is_roundtrip": true
        },
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "256"s );
        const vector<Node> &stops = req.at("stops"s).AsArray();
        ASSERT_EQUAL( stops.at(0).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( stops.at(1).AsString(), "Biryusinka"s );
        ASSERT_EQUAL( stops.at(2).AsString(), "Universam"s );
        ASSERT_EQUAL( stops.at(3).AsString(), "Biryulyovo Tovarnaya"s );
        ASSERT_EQUAL( stops.at(4).AsString(), "Biryulyovo Passazhirskaya"s );
        ASSERT_EQUAL( stops.at(5).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( req.at("is_roundtrip"s).AsBool(), true );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "750",
            "stops": [
                "Tolstopaltsevo",
                "Marushkino",
                "Rasskazovka"
            ],
            "is_roundtrip": false
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "750"s );
        const vector<Node> &stops = req.at("stops"s).AsArray();
        ASSERT_EQUAL( stops.at(0).AsString(), "Tolstopaltsevo"s );
        ASSERT_EQUAL( stops.at(1).AsString(), "Marushkino"s );
        ASSERT_EQUAL( stops.at(2).AsString(), "Rasskazovka"s );
        ASSERT_EQUAL( req.at("is_roundtrip"s).AsBool(), false );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.333324,
            "name": "Rasskazovka",
            "latitude": 55.632761
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 0U );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.333324) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.632761) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Rasskazovka"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Rossoshanskaya ulitsa": 7500,
                "Biryusinka": 1800,
                "Universam": 2400
            },
            "longitude": 37.6517,
            "name": "Biryulyovo Zapadnoye",
            "latitude": 55.574371
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 3U );
        ASSERT_EQUAL( road_distances.at("Rossoshanskaya ulitsa"s).AsInt(), 7500 );
        ASSERT_EQUAL( road_distances.at("Biryusinka"s).AsInt(), 1800 );
        ASSERT_EQUAL( road_distances.at("Universam"s).AsInt(), 2400 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.6517) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.574371) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Zapadnoye"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Universam": 750
            },
            "longitude": 37.64839,
            "name": "Biryusinka",
            "latitude": 55.581065
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Universam"s).AsInt(), 750 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.64839) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.581065) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryusinka"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Rossoshanskaya ulitsa": 5600,
                "Biryulyovo Tovarnaya": 900
            },
            "longitude": 37.645687,
            "name": "Universam",
            "latitude": 55.587655
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 2U );
        ASSERT_EQUAL( road_distances.at("Rossoshanskaya ulitsa"s).AsInt(), 5600 );
        ASSERT_EQUAL( road_distances.at("Biryulyovo Tovarnaya"s).AsInt(), 900 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.645687) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.587655) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Universam"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Biryulyovo Passazhirskaya": 1300
            },
            "longitude": 37.653656,
            "name": "Biryulyovo Tovarnaya",
            "latitude": 55.592028
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Biryulyovo Passazhirskaya"s).AsInt(), 1300 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.653656) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.592028) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Tovarnaya"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {
                "Biryulyovo Zapadnoye": 1200
            },
            "longitude": 37.659164,
            "name": "Biryulyovo Passazhirskaya",
            "latitude": 55.580999
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 1U );
        ASSERT_EQUAL( road_distances.at("Biryulyovo Zapadnoye"s).AsInt(), 1200 );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.659164) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.580999) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Passazhirskaya"s );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "828",
            "stops": [
                "Biryulyovo Zapadnoye",
                "Universam",
                "Rossoshanskaya ulitsa",
                "Biryulyovo Zapadnoye"
            ],
            "is_roundtrip": true
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "828"s );
        const vector<Node> &stops = req.at("stops"s).AsArray();
        ASSERT_EQUAL( stops.size(), 4U );
        ASSERT_EQUAL( stops.at(0).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( stops.at(1).AsString(), "Universam"s );
        ASSERT_EQUAL( stops.at(2).AsString(), "Rossoshanskaya ulitsa"s );
        ASSERT_EQUAL( stops.at(3).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( req.at("is_roundtrip"s).AsBool(), true );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.605757,
            "name": "Rossoshanskaya ulitsa",
            "latitude": 55.595579
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 0U );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.605757) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.595579) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Rossoshanskaya ulitsa"s );
    }
    {
        /*
        {
            "type": "Stop",
            "road_distances": {},
            "longitude": 37.603831,
            "name": "Prazhskaya",
            "latitude": 55.611678
        }
        */
        const map<string, Node> &req = base_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        const map<string, Node> &road_distances = req.at("road_distances"s).AsMap();
        ASSERT_EQUAL( road_distances.size(), 0U );
        ASSERT( AssertDouble(req.at("longitude"s).AsDouble(), 37.603831) );
        ASSERT( AssertDouble(req.at("latitude"s).AsDouble(), 55.611678) );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Prazhskaya"s );
    }

    node_num = 0U;
    const vector<Node> &stat_requests = root.at("stat_requests"s).AsArray();

    {
        /*
        {
            "type": "Bus",
            "name": "256",
            "id": 1965312327
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "256"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 1965312327 );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "750",
            "id": 519139350
        },
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "750"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 519139350 );
    }
    {
        /*
        {
            "type": "Bus",
            "name": "751",
            "id": 194217464
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Bus"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "751"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 194217464 );
    }
    {
        /*
        {
            "type": "Stop",
            "name": "Samara",
            "id": 746888088
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Samara"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 746888088 );
    }
    {
        /*
        {
            "type": "Stop",
            "name": "Prazhskaya",
            "id": 65100610
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Prazhskaya"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 65100610 );
    }
    {
        /*
        {
            "type": "Stop",
            "name": "Biryulyovo Zapadnoye",
            "id": 1042838872
        }
        */
        const map<string, Node> &req = stat_requests.at(node_num++).AsMap();
        ASSERT_EQUAL( req.at("type"s).AsString(), "Stop"s );
        ASSERT_EQUAL( req.at("name"s).AsString(), "Biryulyovo Zapadnoye"s );
        ASSERT_EQUAL( req.at("id"s).AsInt(), 1042838872 );
    }
}

void TestParse()
{
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.574371,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 2
    }
  ]
}
)");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 2,
    "error_message": "not found"
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "Malina Kalina",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.571231,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    },
    {
      "type": "Bus",
      "name": "257",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryusinka",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "258",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya"
      ],
      "is_roundtrip": false
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "Malina Kalina",
      "id": 1965312320
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1965312321
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 1965312322
    },
    {
      "type": "Bus",
      "name": "257",
      "id": 1965312323
    },
    {
      "type": "Bus",
      "name": "258",
      "id": 1965312324
    }
  ]
}
)");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 1965312320,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1965312321,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1965312322,
    "error_message": "not found"
  },
  {
    "request_id": 1965312323,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1965312324,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.35,
      "name": "Marushkino"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Parushka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.35,
      "name": "Sasushka"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Tolstopaltsevo"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "755",
      "stops": [
        "Parushka",
        "Sasushka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "756",
      "stops": [
        "Parushka",
        "Sasushka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "757",
      "stops": [
        "Parushka",
        "Sasushka",
        "Tolstopaltsevo",
        "Marushkino"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "758",
      "stops": [
        "Parushka",
        "Sasushka",
        "Marushkino",
        "Tolstopaltsevo"
      ],
      "is_roundtrip": false
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "256",
      "id": 2
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 3
    },
    {
      "type": "Bus",
      "name": "755",
      "id": 4
    },
    {
      "type": "Bus",
      "name": "756",
      "id": 5
    },
    {
      "type": "Bus",
      "name": "757",
      "id": 6
    },
    {
      "type": "Bus",
      "name": "758",
      "id": 7
    }
  ]
}
)");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 2,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 3,
    "error_message": "not found"
  },
  {
    "request_id": 4,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 5,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 6,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 7,
    "stop_count": 7,
    "unique_stop_count": 4,
    "route_length": 0,
    "curvature": 0
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.31,
      "name": "X1"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.32,
      "name": "X2"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.33,
      "name": "X3"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.34,
      "name": "X4"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.35,
      "name": "X5"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.36,
      "name": "X6"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.37,
      "name": "X7"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.38,
      "name": "X8"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.39,
      "name": "X9"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 56.60,
      "longitude": 50.40,
      "name": "X10 + X1 + X5 + X9"
    },
    {
      "type": "Bus",
      "name": "X",
      "stops": [
        "X1",
        "X2",
        "X3",
        "X4",
        "X5",
        "X6",
        "X7",
        "X8",
        "X9",
        "X10 + X1 + X5 + X9"
      ],
      "is_roundtrip": false
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "X",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "X",
      "id": 1
    }
  ]
}
)");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 19,
    "unique_stop_count": 10,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 19,
    "unique_stop_count": 10,
    "route_length": 0,
    "curvature": 0
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"(
{ "routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.574371,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Universam",
        "Rossoshanskaya ulitsa",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595579,
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611678,
      "longitude": 37.603831,
      "name": "Prazhskaya"
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 2
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 3
    },
    {
      "type": "Stop",
      "name": "Prazhskaya",
      "id": 4
    },
    {
      "type": "Stop",
      "name": "Biryulyovo Zapadnoye",
      "id": 5
    }
  ]
}
)");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 1,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 2,
    "error_message": "not found"
  },
  {
    "request_id": 3,
    "error_message": "not found"
  },
  {
    "request_id": 4,
    "buses": []
  },
  {
    "request_id": 5,
    "buses": [
      "256",
      "828"
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40},
  "base_requests": [
    {
      "type": "Stop",
      "road_distances": {
        "Marushkino": 3900
      },
      "latitude": 55.611087,
      "longitude": 37.20829,
      "name": "Tolstopaltsevo"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 9900
      },
      "latitude": 55.595884,
      "longitude": 37.209755,
      "name": "Marushkino"
    },
    {
      "type": "Bus",
      "name": "256",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryusinka",
        "Universam",
        "Biryulyovo Tovarnaya",
        "Biryulyovo Passazhirskaya",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Marushkino",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.632761,
      "longitude": 37.333324,
      "name": "Rasskazovka"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 7500,
        "Biryusinka": 1800,
        "Universam": 2400
      },
      "latitude": 55.574371,
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 750
      },
      "latitude": 55.581065,
      "longitude": 37.64839,
      "name": "Biryusinka"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 5600,
        "Biryulyovo Tovarnaya": 900
      },
      "latitude": 55.587655,
      "longitude": 37.645687,
      "name": "Universam"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Passazhirskaya": 1300
      },
      "latitude": 55.592028,
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya"
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Zapadnoye": 1200
      },
      "latitude": 55.580999,
      "longitude": 37.659164,
      "name": "Biryulyovo Passazhirskaya"
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Universam",
        "Rossoshanskaya ulitsa",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.595579,
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa"
    },
    {
      "type": "Stop",
      "road_distances": {},
      "latitude": 55.611678,
      "longitude": 37.603831,
      "name": "Prazhskaya"
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "256",
      "id": 0
    },
    {
      "type": "Bus",
      "name": "750",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "751",
      "id": 2
    },
    {
      "type": "Stop",
      "name": "Samara",
      "id": 3
    },
    {
      "type": "Stop",
      "name": "Prazhskaya",
      "id": 4
    },
    {
      "type": "Stop",
      "name": "Biryulyovo Zapadnoye",
      "id": 5
    }
  ]
}
)");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 0,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 5950,
    "curvature": 1.36124
  },
  {
    "request_id": 1,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 27600,
    "curvature": 1.31808
  },
  {
    "request_id": 2,
    "error_message": "not found"
  },
  {
    "request_id": 3,
    "error_message": "not found"
  },
  {
    "request_id": 4,
    "buses": []
  },
  {
    "request_id": 5,
    "buses": [
      "256",
      "828"
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40}, "base_requests": [{"latitude": 55.611087, "longitude": 37.20829, "type": "Stop", "road_distances": {"+++": 3900}, "name": "Tolstopaltsevo"}, {"type": "Stop", "name": "+++", "latitude": 55.595884, "longitude": 37.209755, "road_distances": {"Rasskazovka": 9900}}, {"type": "Bus", "name": "256", "stops": ["Biryulyovo Zapadnoye", "Biryusinka", "Universam", "Biryulyovo Tovarnaya", "Biryulyovo Passazhirskaya", "Biryulyovo Zapadnoye"], "is_roundtrip": true}, {"type": "Bus", "name": "750", "is_roundtrip": false, "stops": ["Tolstopaltsevo", "Marushkino", "Rasskazovka"]}, {"type": "Stop", "name": "Rasskazovka", "latitude": 55.632761, "longitude": 37.333324, "road_distances": {}}, {"type": "Stop", "name": "Biryulyovo Zapadnoye", "latitude": 55.574371, "longitude": 37.6517, "road_distances": {"Biryusinka": 1800, "Universam": 2400, "Rossoshanskaya ulitsa": 7500}}, {"type": "Stop", "name": "Biryusinka", "latitude": 55.581065, "longitude": 37.64839, "road_distances": {"Universam": 750}}, {"type": "Stop", "name": "Universam", "latitude": 55.587655, "longitude": 37.645687, "road_distances": {"Biryulyovo Tovarnaya": 900, "Rossoshanskaya ulitsa": 5600}}, {"type": "Stop", "name": "Biryulyovo Tovarnaya", "latitude": 55.592028, "longitude": 37.653656, "road_distances": {"Biryulyovo Passazhirskaya": 1300}}, {"type": "Stop", "name": "Biryulyovo Passazhirskaya", "latitude": 55.580999, "longitude": 37.659164, "road_distances": {"Biryulyovo Zapadnoye": 1200}}, {"type": "Bus", "name": "828", "stops": ["Biryulyovo Zapadnoye", "Universam", "Rossoshanskaya ulitsa", "Biryulyovo Zapadnoye"], "is_roundtrip": true}, {"type": "Stop", "name": "Rossoshanskaya ulitsa", "latitude": 55.595579, "longitude": 37.605757, "road_distances": {}}, {"type": "Stop", "name": "Prazhskaya", "latitude": 55.611678, "longitude": 37.603831, "road_distances": {}}], "stat_requests": [{"id": 599708471, "type": "Bus", "name": "256"}, {"id": 2059890189, "type": "Bus", "name": "750"}, {"id": 222058974, "type": "Bus", "name": "751"}, {"id": 1038752326, "type": "Stop", "name": "Samara"}, {"id": 986197773, "type": "Stop", "name": "Prazhskaya"}, {"id": 932894250, "type": "Stop", "name": "Biryulyovo Zapadnoye"}]})");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 599708471,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 5950,
    "curvature": 1.36124
  },
  {
    "request_id": 2059890189,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 0,
    "curvature": 0
  },
  {
    "request_id": 222058974,
    "error_message": "not found"
  },
  {
    "request_id": 1038752326,
    "error_message": "not found"
  },
  {
    "request_id": 986197773,
    "buses": []
  },
  {
    "request_id": 932894250,
    "buses": [
      "256",
      "828"
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({"routing_settings": {"bus_wait_time": 6, "bus_velocity": 40}, "base_requests": [{"type": "Stop", "name": "A", "latitude": 0.5, "longitude": -1, "road_distances": {"B": 100000}}, {"type": "Stop", "name": "B", "latitude": 0, "longitude": -1.1, "road_distances": {}}, {"type": "Bus", "name": "256", "stops": ["B", "A"], "is_roundtrip": false}], "stat_requests": [{"id": 1317873202, "type": "Bus", "name": "256"}, {"id": 853807922, "type": "Stop", "name": "A"}, {"id": 1416389015, "type": "Stop", "name": "B"}, {"id": 1021316791, "type": "Stop", "name": "C"}]})");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 1317873202,
    "stop_count": 3,
    "unique_stop_count": 2,
    "route_length": 200000,
    "curvature": 1.76372
  },
  {
    "request_id": 853807922,
    "buses": [
      "256"
    ]
  },
  {
    "request_id": 1416389015,
    "buses": [
      "256"
    ]
  },
  {
    "request_id": 1021316791,
    "error_message": "not found"
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({
  "routing_settings": {
    "bus_wait_time": 6,
    "bus_velocity": 40
  },
  "base_requests": [
    {
      "type": "Bus",
      "name": "297",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "635",
      "stops": [
        "Biryulyovo Tovarnaya",
        "Universam",
        "Prazhskaya"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Tovarnaya": 2600
      },
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye",
      "latitude": 55.574371
    },
    {
      "type": "Stop",
      "road_distances": {
        "Prazhskaya": 4650,
        "Biryulyovo Tovarnaya": 1380,
        "Biryulyovo Zapadnoye": 2500
      },
      "longitude": 37.645687,
      "name": "Universam",
      "latitude": 55.587655
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 890
      },
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya",
      "latitude": 55.592028
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.603938,
      "name": "Prazhskaya",
      "latitude": 55.611717
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "297",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "635",
      "id": 2
    },
    {
      "type": "Stop",
      "name": "Universam",
      "id": 3
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Universam",
      "id": 4
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Prazhskaya",
      "id": 5
    }
  ]
})");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 1,
    "stop_count": 4,
    "unique_stop_count": 3,
    "route_length": 5990,
    "curvature": 1.42963
  },
  {
    "request_id": 2,
    "stop_count": 5,
    "unique_stop_count": 3,
    "route_length": 11570,
    "curvature": 1.30156
  },
  {
    "request_id": 3,
    "buses": [
      "297",
      "635"
    ]
  },
  {
    "request_id": 4,
    "total_time": 11.235,
    "items": [
        {
            "time": 6,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 2,
            "bus": "297",
            "type": "Bus",
            "time": 5.235
        }
    ]
  },
  {
    "request_id": 5,
    "total_time": 24.21,
    "items": [
        {
            "time": 6,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 2,
            "bus": "297",
            "type": "Bus",
            "time": 5.235
        },
        {
            "time": 6,
            "type": "Wait",
            "stop_name": "Universam"
        },
        {
            "span_count": 1,
            "bus": "635",
            "type": "Bus",
            "time": 6.975
        }
    ]
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect);
    }
    {
        istringstream iss(R"({
  "routing_settings": {
    "bus_wait_time": 2,
    "bus_velocity": 30
  },
  "base_requests": [
    {
      "type": "Bus",
      "name": "297",
      "stops": [
        "Biryulyovo Zapadnoye",
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryusinka",
        "Apteka",
        "Biryulyovo Zapadnoye"
      ],
      "is_roundtrip": true
    },
    {
      "type": "Bus",
      "name": "635",
      "stops": [
        "Biryulyovo Tovarnaya",
        "Universam",
        "Biryusinka",
        "TETs 26",
        "Pokrovskaya",
        "Prazhskaya"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Bus",
      "name": "828",
      "stops": [
        "Biryulyovo Zapadnoye",
        "TETs 26",
        "Biryusinka",
        "Universam",
        "Pokrovskaya",
        "Rossoshanskaya ulitsa"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Tovarnaya": 2600,
        "TETs 26": 1100
      },
      "longitude": 37.6517,
      "name": "Biryulyovo Zapadnoye",
      "latitude": 55.574371
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryusinka": 760,
        "Biryulyovo Tovarnaya": 1380,
        "Pokrovskaya": 2460
      },
      "longitude": 37.645687,
      "name": "Universam",
      "latitude": 55.587655
    },
    {
      "type": "Stop",
      "road_distances": {
        "Universam": 890
      },
      "longitude": 37.653656,
      "name": "Biryulyovo Tovarnaya",
      "latitude": 55.592028
    },
    {
      "type": "Stop",
      "road_distances": {
        "Apteka": 210,
        "TETs 26": 400
      },
      "longitude": 37.64839,
      "name": "Biryusinka",
      "latitude": 55.581065
    },
    {
      "type": "Stop",
      "road_distances": {
        "Biryulyovo Zapadnoye": 1420
      },
      "longitude": 37.652296,
      "name": "Apteka",
      "latitude": 55.580023
    },
    {
      "type": "Stop",
      "road_distances": {
        "Pokrovskaya": 2850
      },
      "longitude": 37.642258,
      "name": "TETs 26",
      "latitude": 55.580685
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rossoshanskaya ulitsa": 3140
      },
      "longitude": 37.635517,
      "name": "Pokrovskaya",
      "latitude": 55.603601
    },
    {
      "type": "Stop",
      "road_distances": {
        "Pokrovskaya": 3210
      },
      "longitude": 37.605757,
      "name": "Rossoshanskaya ulitsa",
      "latitude": 55.595579
    },
    {
      "type": "Stop",
      "road_distances": {
        "Pokrovskaya": 2260
      },
      "longitude": 37.603938,
      "name": "Prazhskaya",
      "latitude": 55.611717
    },
    {
      "type": "Bus",
      "name": "750",
      "stops": [
        "Tolstopaltsevo",
        "Rasskazovka"
      ],
      "is_roundtrip": false
    },
    {
      "type": "Stop",
      "road_distances": {
        "Rasskazovka": 13800
      },
      "longitude": 37.20829,
      "name": "Tolstopaltsevo",
      "latitude": 55.611087
    },
    {
      "type": "Stop",
      "road_distances": {},
      "longitude": 37.333324,
      "name": "Rasskazovka",
      "latitude": 55.632761
    }
  ],
  "stat_requests": [
    {
      "type": "Bus",
      "name": "297",
      "id": 1
    },
    {
      "type": "Bus",
      "name": "635",
      "id": 2
    },
    {
      "type": "Bus",
      "name": "828",
      "id": 3
    },
    {
      "type": "Stop",
      "name": "Universam",
      "id": 4
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Apteka",
      "id": 5
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Pokrovskaya",
      "id": 6
    },
    {
      "type": "Route",
      "from": "Biryulyovo Tovarnaya",
      "to": "Pokrovskaya",
      "id": 7
    },
    {
      "type": "Route",
      "from": "Biryulyovo Tovarnaya",
      "to": "Biryulyovo Zapadnoye",
      "id": 8
    },
    {
      "type": "Route",
      "from": "Biryulyovo Tovarnaya",
      "to": "Prazhskaya",
      "id": 9
    },
    {
      "type": "Route",
      "from": "Apteka",
      "to": "Biryulyovo Tovarnaya",
      "id": 10
    },
    {
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Tolstopaltsevo",
      "id": 11
    }
  ]
})");

        ostringstream oss;
        DataBase db;
        Parse(iss, oss, db);

        istringstream iss_expect(R"([
  {
    "request_id": 1,
    "stop_count": 6,
    "unique_stop_count": 5,
    "route_length": 5880,
    "curvature": 1.36159
  },
  {
    "request_id": 2,
    "stop_count": 11,
    "unique_stop_count": 6,
    "route_length": 14810,
    "curvature": 1.12195
  },
  {
    "request_id": 3,
    "stop_count": 11,
    "unique_stop_count": 6,
    "route_length": 15790,
    "curvature": 1.31245
  },
  {
    "request_id": 4,
    "buses": [
      "297",
      "635",
      "828"
    ]
  },
  {
    "request_id": 5,
    "total_time": 7.42,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 2,
            "bus": "828",
            "type": "Bus",
            "time": 3
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryusinka"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 0.42
        }
    ]
  },
  {
    "request_id": 6,
    "total_time": 11.44,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 4,
            "bus": "828",
            "type": "Bus",
            "time": 9.44
        }
    ]
  },
  {
    "request_id": 7,
    "total_time": 10.7,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Tovarnaya"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 1.78
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Universam"
        },
        {
            "span_count": 1,
            "bus": "828",
            "type": "Bus",
            "time": 4.92
        }
    ]
  },
  {
    "request_id": 8,
    "total_time": 8.56,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Tovarnaya"
        },
        {
            "span_count": 4,
            "bus": "297",
            "type": "Bus",
            "time": 6.56
        }
    ]
  },
  {
    "request_id": 9,
    "total_time": 16.32,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Tovarnaya"
        },
        {
            "span_count": 5,
            "bus": "635",
            "type": "Bus",
            "time": 14.32
        }
    ]
  },
  {
    "request_id": 10,
    "total_time": 12.04,
    "items": [
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Apteka"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 2.84
        },
        {
            "time": 2,
            "type": "Wait",
            "stop_name": "Biryulyovo Zapadnoye"
        },
        {
            "span_count": 1,
            "bus": "297",
            "type": "Bus",
            "time": 5.2
        }
    ]
  },
  {
    "request_id": 11,
    "error_message": "not found"
  }
])");

        string str = oss.str();
        string str_expect = iss_expect.str();

        ASSERT_EQUAL(str, str_expect); // TODO: bring back
    }
    {
        ifstream input("src/long.json");
        DataBase db;
        LOG_DURATION("Long Test");
        Parse(input, cout, db);
    }
}


void TestParseRouteQuery()
{
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});
        StopPtr stop4 = make_shared<Stop>(Stop{"4"});
        StopPtr stop5 = make_shared<Stop>(Stop{"5"});
        StopPtr stop6 = make_shared<Stop>(Stop{"6"});
        StopPtr stop7 = make_shared<Stop>(Stop{"7"});
        StopPtr stop8 = make_shared<Stop>(Stop{"8"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop3, stop4, stop5}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop3, stop6}});
        BusPtr bus4 = make_shared<Bus>(Bus{"844", {stop7, stop8}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7, stop8};
        db.buses = {bus1, bus2, bus3, bus4};

        db.road_route_length[stop1][stop2] = 1000;
        db.road_route_length[stop2][stop1] = 1000;
        db.road_route_length[stop2][stop3] = 500;
        db.road_route_length[stop3][stop2] = 500;
        db.road_route_length[stop3][stop4] = 800;
        db.road_route_length[stop4][stop3] = 800;
        db.road_route_length[stop4][stop5] = 300;
        db.road_route_length[stop5][stop4] = 300;
        db.road_route_length[stop3][stop6] = 200;
        db.road_route_length[stop6][stop3] = 200;
        db.road_route_length[stop7][stop8] = 100;
        db.road_route_length[stop8][stop7] = 200;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop2, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 7.5));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 1.5));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 8.25));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop3, stop1, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 8.25));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop4, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 15.45));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus2);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 1.2));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop6, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 14.55));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus3);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop6, stop1, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 14.55));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop6);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus3);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 2.25));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop7, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop1, stop8, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop8, stop1, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop8, stop2, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop7, stop4, db, router);
            ASSERT(not answer.has_value());
            answer = ParseRouteQuery(stop7, stop3, db, router);
            ASSERT(not answer.has_value());
        }
    }

    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});
        StopPtr stop4 = make_shared<Stop>(Stop{"4"});
        StopPtr stop5 = make_shared<Stop>(Stop{"5"});
        StopPtr stop6 = make_shared<Stop>(Stop{"6"});
        StopPtr stop7 = make_shared<Stop>(Stop{"7"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop7}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop1, stop3, stop7}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop1, stop4, stop7}});
        BusPtr bus4 = make_shared<Bus>(Bus{"844", {stop1, stop5, stop7}});
        BusPtr bus5 = make_shared<Bus>(Bus{"845", {stop1, stop6, stop7}});

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4, stop5, stop6, stop7};
        db.buses = {bus1, bus2, bus3, bus4, bus5};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop1] = 100;
        db.road_route_length[stop1][stop3] = 200;
        db.road_route_length[stop3][stop1] = 200;
        db.road_route_length[stop1][stop4] = 300;
        db.road_route_length[stop4][stop1] = 300;
        db.road_route_length[stop1][stop5] = 99;
        db.road_route_length[stop5][stop1] = 99;
        db.road_route_length[stop1][stop6] = 400;
        db.road_route_length[stop6][stop1] = 400;

        db.road_route_length[stop2][stop7] = 1000;
        db.road_route_length[stop3][stop7] = 1000;
        db.road_route_length[stop4][stop7] = 1000;
        db.road_route_length[stop5][stop7] = 1000;
        db.road_route_length[stop6][stop7] = 1000;
        db.road_route_length[stop7][stop2] = 1000;
        db.road_route_length[stop7][stop3] = 1000;
        db.road_route_length[stop7][stop4] = 1000;
        db.road_route_length[stop7][stop5] = 1000;
        db.road_route_length[stop7][stop6] = 1000;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop7, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 0.148 + 1.5));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus4);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 0.148 + 1.5));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop7, stop1, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 0.148 + 1.5));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop7);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus4);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 0.148 + 1.5));
        }
        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop6, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 0.6 + 6 + 0.3));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop6);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus5);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.6));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus2);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
        }
    }

    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop2, stop3}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop1, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3};
        db.buses = {bus1, bus2, bus3};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop1] = 100;

        db.road_route_length[stop2][stop3] = 200;
        db.road_route_length[stop3][stop2] = 200;

        db.road_route_length[stop1][stop3] = 4310;
        db.road_route_length[stop3][stop1] = 4310;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 6.45));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.15));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop2);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus2);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 0.3));
        }
    }

    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2}});
        BusPtr bus2 = make_shared<Bus>(Bus{"842", {stop2, stop3}});
        BusPtr bus3 = make_shared<Bus>(Bus{"843", {stop1, stop3}});

        DataBase db;
        db.stops = {stop1, stop2, stop3};
        db.buses = {bus1, bus2, bus3};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop1] = 100;

        db.road_route_length[stop2][stop3] = 200;
        db.road_route_length[stop3][stop2] = 200;

        db.road_route_length[stop1][stop3] = 4295;
        db.road_route_length[stop3][stop1] = 4295;

        db.CreateInfo(6/*bus_wait_time*/, 40.0/*bus_velocity km/hour*/);

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop1, stop3, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 6 + 6.44));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus3);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 6.44));
        }
    }
    {
        StopPtr stop1 = make_shared<Stop>(Stop{"1"});
        StopPtr stop2 = make_shared<Stop>(Stop{"2"});
        StopPtr stop3 = make_shared<Stop>(Stop{"3"});
        StopPtr stop4 = make_shared<Stop>(Stop{"4"});

        BusPtr bus1 = make_shared<Bus>(Bus{"841", {stop1, stop2, stop3, stop4, stop1}});
        bus1->ring = true;

        DataBase db;
        db.stops = {stop1, stop2, stop3, stop4};
        db.buses = {bus1};

        db.road_route_length[stop1][stop2] = 100;
        db.road_route_length[stop2][stop3] = 200;
        db.road_route_length[stop3][stop4] = 300;
        db.road_route_length[stop4][stop1] = 400;

        db.CreateInfo(10/*bus_wait_time*/, 6.0/*bus_velocity km/hour*/);
        // 100 метров/минуту

        Router router{db.graph};

        {
            std::optional<RouteQueryAnswer> answer = ParseRouteQuery(stop3, stop2, db, router);
            ASSERT(answer.has_value());
            ASSERT(AssertDouble(answer->total_time, 10 + 7 + 10 + 1));
            const vector<Item> &items = answer->items;
            const WaitItem *wait_item = get_if<WaitItem>(&items[0]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop3);
            const BusItem *bus_item = get_if<BusItem>(&items[1]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 2U);
            ASSERT(AssertDouble(bus_item->time, 7));
            wait_item = get_if<WaitItem>(&items[2]);
            ASSERT(wait_item);
            ASSERT_EQUAL(wait_item->stop, stop1);
            bus_item = get_if<BusItem>(&items[3]);
            ASSERT(bus_item);
            ASSERT_EQUAL(bus_item->bus, bus1);
            ASSERT_EQUAL(bus_item->span_count, 1U);
            ASSERT(AssertDouble(bus_item->time, 1));
        }
    }
}

void Test15Failed()
{
    // ifstream input("src/test15failed.json");
    ifstream input("src/test15failed.json");
    // ifstream input("sniffed_cut.json");
    DataBase db;

    Parse(input, cout, db);
}