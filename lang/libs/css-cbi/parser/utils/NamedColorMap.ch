import "@std/unordered_map.ch"
import "@std/string_view.ch"

struct NamedColorMap {

    var map : std::unordered_map<std::string_view, bool>

    @make
    func make() {
        map.insert(std::string_view("black"), true);
        map.insert(std::string_view("silver"), true);
        map.insert(std::string_view("gray"), true);
        map.insert(std::string_view("white"), true);
        map.insert(std::string_view("maroon"), true);
        map.insert(std::string_view("red"), true);
        map.insert(std::string_view("purple"), true);
        map.insert(std::string_view("fuchsia"), true);
        map.insert(std::string_view("green"), true);
        map.insert(std::string_view("lime"), true);
        map.insert(std::string_view("olive"), true);
        map.insert(std::string_view("yellow"), true);
        map.insert(std::string_view("navy"), true);
        map.insert(std::string_view("blue"), true);
        map.insert(std::string_view("teal"), true);
        map.insert(std::string_view("aqua"), true);
        map.insert(std::string_view("aliceblue"), true);
        map.insert(std::string_view("antiquewhite"), true);
        map.insert(std::string_view("aqua"), true);
        map.insert(std::string_view("aquamarine"), true);
        map.insert(std::string_view("azure"), true);
        map.insert(std::string_view("beige"), true);
        map.insert(std::string_view("bisque"), true);
        map.insert(std::string_view("black"), true);
        map.insert(std::string_view("blanchedalmond"), true);
        map.insert(std::string_view("blue"), true);
        map.insert(std::string_view("blueviolet"), true);
        map.insert(std::string_view("brown"), true);
        map.insert(std::string_view("burlywood"), true);
        map.insert(std::string_view("cadetblue"), true);
        map.insert(std::string_view("chartreuse"), true);
        map.insert(std::string_view("chocolate"), true);
        map.insert(std::string_view("coral"), true);
        map.insert(std::string_view("cornflowerblue"), true);
        map.insert(std::string_view("cornsilk"), true);
        map.insert(std::string_view("crimson"), true);
        map.insert(std::string_view("cyan"), true);
        map.insert(std::string_view("darkblue"), true);
        map.insert(std::string_view("darkcyan"), true);
        map.insert(std::string_view("darkgoldenrod"), true);
        map.insert(std::string_view("darkgray"), true);
        map.insert(std::string_view("darkgreen"), true);
        map.insert(std::string_view("darkgrey"), true);
        map.insert(std::string_view("darkkhaki"), true);
        map.insert(std::string_view("darkmagenta"), true);
        map.insert(std::string_view("darkolivegreen"), true);
        map.insert(std::string_view("darkorange"), true);
        map.insert(std::string_view("darkorchid"), true);
        map.insert(std::string_view("darkred"), true);
        map.insert(std::string_view("darksalmon"), true);
        map.insert(std::string_view("darkseagreen"), true);
        map.insert(std::string_view("darkslateblue"), true);
        map.insert(std::string_view("darkslategray"), true);
        map.insert(std::string_view("darkslategrey"), true);
        map.insert(std::string_view("darkturquoise"), true);
        map.insert(std::string_view("darkviolet"), true);
        map.insert(std::string_view("deeppink"), true);
        map.insert(std::string_view("deepskyblue"), true);
        map.insert(std::string_view("dimgray"), true);
        map.insert(std::string_view("dimgrey"), true);
        map.insert(std::string_view("dodgerblue"), true);
        map.insert(std::string_view("firebrick"), true);
        map.insert(std::string_view("floralwhite"), true);
        map.insert(std::string_view("forestgreen"), true);
        map.insert(std::string_view("fuchsia"), true);
        map.insert(std::string_view("gainsboro"), true);
        map.insert(std::string_view("ghostwhite"), true);
        map.insert(std::string_view("gold"), true);
        map.insert(std::string_view("goldenrod"), true);
        map.insert(std::string_view("gray"), true);
        map.insert(std::string_view("green"), true);
        map.insert(std::string_view("greenyellow"), true);
        map.insert(std::string_view("grey"), true);
        map.insert(std::string_view("honeydew"), true);
        map.insert(std::string_view("hotpink"), true);
        map.insert(std::string_view("indianred"), true);
        map.insert(std::string_view("indigo"), true);
        map.insert(std::string_view("ivory"), true);
        map.insert(std::string_view("khaki"), true);
        map.insert(std::string_view("lavender"), true);
        map.insert(std::string_view("lavenderblush"), true);
        map.insert(std::string_view("lawngreen"), true);
        map.insert(std::string_view("lemonchiffon"), true);
        map.insert(std::string_view("lightblue"), true);
        map.insert(std::string_view("lightcoral"), true);
        map.insert(std::string_view("lightcyan"), true);
        map.insert(std::string_view("lightgoldenrodyellow"), true);
        map.insert(std::string_view("lightgray"), true);
        map.insert(std::string_view("lightgreen"), true);
        map.insert(std::string_view("lightgrey"), true);
        map.insert(std::string_view("lightpink"), true);
        map.insert(std::string_view("lightsalmon"), true);
        map.insert(std::string_view("lightseagreen"), true);
        map.insert(std::string_view("lightskyblue"), true);
        map.insert(std::string_view("lightslategray"), true);
        map.insert(std::string_view("lightslategrey"), true);
        map.insert(std::string_view("lightsteelblue"), true);
        map.insert(std::string_view("lightyellow"), true);
        map.insert(std::string_view("lime"), true);
        map.insert(std::string_view("limegreen"), true);
        map.insert(std::string_view("linen"), true);
        map.insert(std::string_view("magenta"), true);
        map.insert(std::string_view("maroon"), true);
        map.insert(std::string_view("mediumaquamarine"), true);
        map.insert(std::string_view("mediumblue"), true);
        map.insert(std::string_view("mediumorchid"), true);
        map.insert(std::string_view("mediumpurple"), true);
        map.insert(std::string_view("mediumseagreen"), true);
        map.insert(std::string_view("mediumslateblue"), true);
        map.insert(std::string_view("mediumspringgreen"), true);
        map.insert(std::string_view("mediumturquoise"), true);
        map.insert(std::string_view("mediumvioletred"), true);
        map.insert(std::string_view("midnightblue"), true);
        map.insert(std::string_view("mintcream"), true);
        map.insert(std::string_view("mistyrose"), true);
        map.insert(std::string_view("moccasin"), true);
        map.insert(std::string_view("navajowhite"), true);
        map.insert(std::string_view("navy"), true);
        map.insert(std::string_view("oldlace"), true);
        map.insert(std::string_view("olive"), true);
        map.insert(std::string_view("olivedrab"), true);
        map.insert(std::string_view("orange"), true);
        map.insert(std::string_view("orangered"), true);
        map.insert(std::string_view("orchid"), true);
        map.insert(std::string_view("palegoldenrod"), true);
        map.insert(std::string_view("palegreen"), true);
        map.insert(std::string_view("paleturquoise"), true);
        map.insert(std::string_view("palevioletred"), true);
        map.insert(std::string_view("papayawhip"), true);
        map.insert(std::string_view("peachpuff"), true);
        map.insert(std::string_view("peru"), true);
        map.insert(std::string_view("pink"), true);
        map.insert(std::string_view("plum"), true);
        map.insert(std::string_view("powderblue"), true);
        map.insert(std::string_view("purple"), true);
        map.insert(std::string_view("red"), true);
        map.insert(std::string_view("rosybrown"), true);
        map.insert(std::string_view("royalblue"), true);
        map.insert(std::string_view("saddlebrown"), true);
        map.insert(std::string_view("salmon"), true);
        map.insert(std::string_view("sandybrown"), true);
        map.insert(std::string_view("seagreen"), true);
        map.insert(std::string_view("seashell"), true);
        map.insert(std::string_view("sienna"), true);
        map.insert(std::string_view("silver"), true);
        map.insert(std::string_view("skyblue"), true);
        map.insert(std::string_view("slateblue"), true);
        map.insert(std::string_view("slategray"), true);
        map.insert(std::string_view("slategrey"), true);
        map.insert(std::string_view("snow"), true);
        map.insert(std::string_view("springgreen"), true);
        map.insert(std::string_view("steelblue"), true);
        map.insert(std::string_view("tan"), true);
        map.insert(std::string_view("teal"), true);
        map.insert(std::string_view("thistle"), true);
        map.insert(std::string_view("tomato"), true);
        map.insert(std::string_view("turquoise"), true);
        map.insert(std::string_view("violet"), true);
        map.insert(std::string_view("wheat"), true);
        map.insert(std::string_view("white"), true);
        map.insert(std::string_view("whitesmoke"), true);
        map.insert(std::string_view("yellow"), true);
        map.insert(std::string_view("yellowgreen"), true);
    }

    func isColor(&self, name : &std::string_view) : bool {
        var value : bool = false;
        return map.find(name, value)
    }

}