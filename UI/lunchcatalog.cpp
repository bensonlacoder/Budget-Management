#include "scumenu.h"

// Lunch and all-day service on Nov 20, 2026. Exceptions have a reference on the
// location or item. See docs/dining-menu.md for the dated operator pages.
namespace ScuMenu {
namespace {
const QString general = QStringLiteral("General offering from the venue guide");
const QString thursday = QStringLiteral("Last published lunch menu, Thu Nov 19, 2026");

Item generalItem(const QString &name) {
    return {name, ReceiptPrice, {}, {}, {}, general};
}

Item drink(const QString &name, int small, int medium, int large) {
    return {name, small, "Size", {{"12 fl oz", small}, {"16 fl oz", medium}, {"20 fl oz", large}}};
}

Item coldFoamDrink(const QString &name, int small, int medium, int large) {
    Item item = drink(name, small, medium, large);
    item.extras = {{"Vanilla cold foam", 175}};
    return item;
}

Item burger(const QString &name, int standard, const QString &reference = {}) {
    return {name, standard, "Side", {{"Regular fries", standard},
            {"Sweet potato fries", ReceiptPrice}, {"Curly fries", ReceiptPrice}}, {}, reference};
}

QList<Item> missionBakery() {
    return {
        {"Blueberry Muffin", 405},
        {"Brownie", 385},
        {"Chocolate Chip Banana Loaf Cake", 485},
        {"Chocolate Chip Cookie", 320},
        {"Chocolate Chip Muffin", 405},
        {"Chocolate Chip Scone", 425},
        {"Chocolate Croissant", 495},
        coldFoamDrink("Cold Brew", 520, 540, 590),
        {"Cranberry Scone", 425},
        {"Croissant", 465},
        {"Double Chocolate Cookie", 320},
        {"El Sur Breakfast Empanada", 889},
        {"El Sur Sweet Empanada", 650},
        {"Espresso", 365, "Size", {{"double", 365}, {"triple", 395}}},
        {"Ham & Cheese Croissant", 540},
        drink("Holiday Spice Latte", 575, 625, 675),
        drink("Hot Americano", 425, 475, 525),
        drink("Hot Cappuccino", 525, 575, 625),
        drink("Hot Caramel Macchiato", 570, 650, 695),
        drink("Hot Chai Latte", 540, 570, 620),
        drink("Hot Chocolate", 425, 475, 525),
        drink("Hot Coffee", 355, 375, 405),
        drink("Hot Havana Cappuccino", 570, 615, 645),
        drink("Hot Latte", 520, 570, 620),
        drink("Hot Matcha Latte", 555, 570, 615),
        drink("Hot Tea", 355, 375, 405),
        drink("Hot White Chocolate Mocha", 590, 645, 695),
        coldFoamDrink("Iced Americano", 425, 475, 525),
        coldFoamDrink("Iced Cappuccino", 525, 575, 625),
        coldFoamDrink("Iced Caramel Macchiato", 570, 650, 695),
        coldFoamDrink("Iced Chai Latte", 540, 570, 620),
        coldFoamDrink("Iced Coffee", 470, 540, 570),
        coldFoamDrink("Iced Havana Cappuccino", 570, 615, 645),
        coldFoamDrink("Iced Holiday Spice Latte", 575, 625, 675),
        coldFoamDrink("Iced Latte", 520, 570, 620),
        coldFoamDrink("Iced London Fog Latte", 540, 570, 620),
        coldFoamDrink("Iced Matcha Latte", 555, 570, 615),
        coldFoamDrink("Iced Mocha", 570, 645, 695),
        coldFoamDrink("Iced Oatmeal Cookie Chai Latte", 595, 645, 675),
        coldFoamDrink("Iced Peppermint Mocha", 595, 645, 695),
        coldFoamDrink("Iced Peppermint White Mocha", 595, 645, 695),
        coldFoamDrink("Iced Tea", 415, 440, 520),
        coldFoamDrink("Iced White Chocolate Mocha", 590, 645, 695),
        drink("Italian Soda", 495, 590, 610),
        drink("Lemonade", 375, 405, 425),
        drink("London Fog Latte", 520, 550, 590),
        {"M&M's Cookie", 320},
        drink("Mocha", 570, 645, 695),
        {"Mocha Butterscotch Muffin", 405},
        drink("Oatmeal Cookie Chai Latte", 595, 645, 675),
        drink("Peppermint Mocha", 595, 645, 695),
        drink("Peppermint White Mocha", 595, 645, 695),
        {"Rice Krispies Treat", ReceiptPrice},
        coldFoamDrink("Shaken Lemonade Tea", 470, 520, 570),
        drink("Steamer", 385, 415, 440),
        {"Sugar Cookie", 320},
        {"Traditional Cappuccino", 500}
    };
}

QList<Item> sidebar() {
    return {
        {"Blueberry Muffin", 405},
        {"Brownie", 385},
        {"Chocolate Chip Banana Loaf Cake", 485},
        {"Chocolate Chip Cookie", 320},
        {"Chocolate Chip Muffin", 405},
        {"Chocolate Chip Scone", 425},
        {"Chocolate Croissant", 495},
        drink("Cold Brew", 540, 565, 620),
        {"Cranberry Scone", 425},
        {"Croissant", 465},
        {"Double Chocolate Cookie", 320},
        {"Espresso", 360},
        {"Ham & Cheese Croissant", 540},
        drink("Holiday Spice Latte", 575, 625, 675),
        drink("Hot Americano", 440, 475, 545),
        drink("Hot Cappuccino", 540, 595, 645),
        drink("Hot Caramel Macchiato", 580, 655, 705),
        drink("Hot Chai Latte", 565, 595, 650),
        drink("Hot Chocolate", 440, 490, 540),
        drink("Hot Coffee", 370, 390, 420),
        drink("Hot Latte", 535, 585, 630),
        drink("Hot Matcha Latte", 555, 570, 615),
        drink("Hot Tea", 365, 385, 415),
        drink("Hot White Chocolate Mocha", 605, 659, 710),
        drink("Iced Americano", 440, 475, 545),
        drink("Iced Cappuccino", 540, 595, 645),
        drink("Iced Caramel Macchiato", 580, 655, 705),
        drink("Iced Chai Latte", 565, 595, 650),
        drink("Iced Coffee", 485, 555, 585),
        drink("Iced Holiday Spice Latte", 575, 625, 675),
        drink("Iced Latte", 535, 585, 630),
        drink("Iced Matcha Latte", 555, 570, 615),
        drink("Iced Mocha", 570, 645, 695),
        drink("Iced Oatmeal Cookie Chai Latte", 595, 645, 675),
        drink("Iced Peppermint Mocha", 595, 645, 695),
        drink("Iced Peppermint White Mocha", 595, 645, 695),
        drink("Iced Tea", 420, 450, 530),
        drink("Iced White Chocolate Mocha", 605, 659, 710),
        drink("Italian Soda", 415, 615, 640),
        drink("Lemonade", 390, 420, 443),
        {"M&M's Cookie", 320},
        drink("Mocha", 570, 645, 695),
        {"Mocha Butterscotch Muffin", 405},
        drink("Oatmeal Cookie Chai Latte", 595, 645, 675),
        drink("Peppermint Mocha", 595, 645, 695),
        drink("Peppermint White Mocha", 595, 645, 695),
        {"Rice Krispies Treat", ReceiptPrice},
        drink("Shaken Lemonade Tea", 490, 529, 579),
        drink("Steamer", 385, 415, 440),
        {"Sugar Cookie", 320},
        {"Traditional Cappuccino", 495}
    };
}

QList<Item> thursdayCafeStaples() {
    return {
        {"Blueberry Muffin", 405},
        {"Brownie", 385},
        {"Chocolate Chip Banana Loaf Cake", 485},
        {"Chocolate Chip Cookie", 320},
        {"Chocolate Chip Muffin", 405},
        {"Chocolate Chip Scone", 425},
        {"Chocolate Croissant", 495},
        {"Cold Brew", ReceiptPrice},
        {"Cranberry Scone", 425},
        {"Croissant", 465},
        {"Double Chocolate Cookie", 320},
        {"Espresso", ReceiptPrice},
        {"Ham & Cheese Croissant", 540},
        {"Hot Americano", ReceiptPrice},
        {"Hot Cappuccino", ReceiptPrice},
        {"Hot Caramel Macchiato", ReceiptPrice},
        {"Hot Chai Latte", ReceiptPrice},
        {"Hot Chocolate", ReceiptPrice},
        {"Hot Coffee", ReceiptPrice},
        {"Hot Latte", ReceiptPrice},
        {"Hot Matcha Latte", ReceiptPrice},
        {"Hot Tea", ReceiptPrice},
        {"Hot White Chocolate Mocha", ReceiptPrice},
        {"Iced Americano", ReceiptPrice},
        {"Iced Cappuccino", ReceiptPrice},
        {"Iced Caramel Macchiato", ReceiptPrice},
        {"Iced Chai Latte", ReceiptPrice},
        {"Iced Coffee", ReceiptPrice},
        {"Iced Latte", ReceiptPrice},
        {"Iced Matcha Latte", ReceiptPrice},
        {"Iced Mocha", ReceiptPrice},
        {"Iced Tea", ReceiptPrice},
        {"Iced White Chocolate Mocha", ReceiptPrice},
        drink("Italian Soda", 495, 590, 610),
        {"Lemonade", 1200},
        {"M&M's Cookie", 320},
        {"Mocha", ReceiptPrice},
        {"Mocha Butterscotch Muffin", 405},
        {"Rice Krispies Treat", ReceiptPrice},
        {"Shaken Lemonade Tea", ReceiptPrice},
        {"Steamer", ReceiptPrice},
        {"Sugar Cookie", 320}
    };
}

QList<Item> cadence() {
    return thursdayCafeStaples() + QList<Item>{
        generalItem("Boxed salad"),
        drink("Cafe De Olla", 450, 475, 500),
        drink("Horchata Oat Cold Brew Latte", 495, 525, 600),
        generalItem("House-made sandwich")
    };
}

QList<Item> sunstream() {
    return thursdayCafeStaples() + QList<Item>{
        drink("Holiday Spice Latte", 575, 625, 675),
        drink("Iced Holiday Spice Latte", 575, 625, 675),
        drink("Iced Oatmeal Cookie Chai Latte", 595, 645, 675),
        drink("Iced Peppermint Mocha", 595, 645, 695),
        drink("Iced Peppermint White Mocha", 595, 645, 695),
        drink("Oatmeal Cookie Chai Latte", 595, 645, 675),
        drink("Peppermint Mocha", 595, 645, 695),
        drink("Peppermint White Mocha", 595, 645, 695)
    };
}

QList<Item> freshBytes() {
    const QList<Choice> toppings{{"Extra boba or jelly", 90}};
    const QList<Choice> milk{{"Whole milk", 790}, {"Oat milk", 840}};
    QList<Item> menu{
        {"Strawnana smoothie", 790},
        {"Sun-Kissed Mango smoothie", 790},
        {"Mixed Berry Blast smoothie", 790},
        {"Peach Paradise smoothie", 790},
        {"Lemon pepper tater tots", 795},
        {"Garlic chili egg noodles", 1058},
        {"Creamy tomato bisque and grilled cheese", 1125},
        {"Vegan tomato bisque and succotash", 950},
        {"Blueberry bacon sliders", 1275},
        {"Spiced shrimp", 1279},
        {"Chorizo mac & cheese", 1125},
        {"Sweet & sour popcorn chicken", 1250},
        {"Raised Glazed Donut", 320},
        {"Raised Chocolate Donut", 320},
        {"Old Fashion Glazed Donut", 320},
        {"Apple Fritter", 360},
        {"Plain Bagel", 310},
        {"Everything Bagel", 310},
        {"Wheat Bagel", 310},
        {"Blueberry Bagel", 310},
        {"Jalapeno Bagel", 310},
        {"Vegan Morning Glory Muffin", 405},
        {"Morning Bun", 470},
        {"Spinach and Feta Croissant", 525},
        {"Cinnamon Twist", 480},
        {"Sweet Cheese Pastry", 490},
        {"Tomato Basil Pizetta", 646},
        {"Blueberry Scone", 425},
        {"Iced Lemon Curd Scone", 425},
        {"Marble Loaf Bread", 485},
        {"Citrus Loaf Bread", 485},
        {"Chocolate Chunk Cookie with Sea Salt", 320},
        {"Strawberry matcha latte", 790, "Milk", milk, toppings},
        {"Brown sugar boba", 790, "Milk", milk, toppings},
        {"Taro honeydew cream", 790, {}, {}, toppings},
        {"Thai milk tea", 790, {}, {}, toppings},
        {"Watermelon black tea", 790, {}, {}, toppings},
        {"Mango lychee green tea", 790, {}, {}, toppings}
    };
    for (auto &item : menu) {
        item.reference = "Lunch menu, Fri Nov 13, 2026";
    }
    menu.prepend({"Chocolate chess pie", 600});
    menu.prepend({"Biscoff pumpkin cheesecake with salted caramel drizzle", 600});
    menu.append(generalItem("Coffee"));
    return menu;
}

}

const QList<Location> &locations() {
    static const QList<Location> catalog{
        {"Marketplace", {}},
        {"Marketplace / Acai", {{"Acai bowl"}}},
        {"Marketplace / Chef's Table", {{"Build your own Korean bibimbap", 1729}},
            "Last published lunch special, Wed Nov 18, 2026"},
        {"Marketplace / Fire Grill", {
            burger("Cheeseburger combo", 1440),
            burger("Bacon cheeseburger combo", 1545),
            {"Signature smash burger", 1545}, {"Fried chicken sandwich", 1545},
            {"Tuna melt", 1475}, {"Chicken tenders with fries (4 pieces)", 1340},
            {"Chicken tenders with fries (3 pieces, value meal)", 1175},
            {"Nashville chicken tenders with fries (3 pieces)", 1410},
            {"Seasonal vegetable sticks with ranch (value meal)", 800},
            {"French fries", 415}, {"Sweet potato fries", 480}, {"Curly fries", 480},
            {"Fountain drink (16 oz)", 260}
        }},
        {"Marketplace / Global Grill", {
            {"Lemongrass tofu bao buns", 1359}, {"Pork belly bao buns", 1579},
            burger("BYO burger", ReceiptPrice, "Student-reported option at Global Grill")
        }},
        {"Marketplace / La Parilla", {
            {"Beans and cheese burrito (value meal)", 700}, {"Chips and pico de gallo", 300},
            {"Chips and guacamole (value meal)", 895}, {"Crispy tacos de papa (value meal)", 1125},
            {"Quesabirria with consommé", 1475}, generalItem("Burrito bowl")
        }},
        {"Marketplace / Poke & Sushi", {
            {"Poke bowl", ReceiptPrice, {}, {}, {{"Half avocado", 155}}}, generalItem("Sushi")
        }},
        {"Marketplace / Simply Oasis", {
            {"Grilled vegetable involtini", 1369}, {"Herb roasted turkey breast", 1589}
        }},
        {"Marketplace / Soup & Salad Bar", {
            {"Salad bar", 690, "Portion", {{"8 oz", 690}, {"15 oz", 1345}}, {{"Avocado", 405}}},
            {"Sweet potato salad"}, {"Soba noodle salad with roasted tofu"}, {"Red cabbage and herb slaw"},
            {"Chicken noodle soup", 525}, {"Coconut carrot ginger soup", 525}
        }},
        {"Marketplace / Spice Market", {
            {"Korean-style Impossible noodle stir-fry", 1479}, {"Korean-style chicken noodle stir-fry", 1689}
        }},
        {"Marketplace / Stacks Deli", {
            {"Deli sandwich"}, {"Chicken Caesar wrap", 1200}, {"Baja chicken wrap (value meal)", 1200}
        }},
        {"Marketplace / The Slice", {
            {"Cheese pizza slice", 670}, {"Pepperoni pizza slice", 735},
            {"Whole cheese pizza", 2870}, {"Whole pepperoni pizza", 3005},
            {"Personal cheese pizza on gluten-free crust", 1080},
            {"Whole pepperoni pizza on gluten-free crust", 1290},
            {"Pizza slice combo (value meal)", 935}, {"Meatball sub (value meal)", 1175},
            {"Garlic pizza sticks (value meal)", 1125}, {"Bronco pizza dog (value meal)", 1200}
        }},
        {"Marketplace / Toast Bar", {{"Strawberry yogurt", 695}, {"Vanilla yogurt", 695}}},
        {"Marketplace / Trattoria", {
            {"Build your own pasta", 825, "Portion", {{"Half", 825}, {"Full", 1190}},
             {{"Grilled chicken", 310}, {"Shrimp", 310}, {"Ground beef", 310},
              {"Italian sausage", 310}, {"Tofu", 310}, {"Bacon", 310}}}
        }},
        {"Mission Bakery & Cafe", missionBakery()},
        {"Cadence Cyber Cafe", cadence(), thursday},
        {"Fresh Bytes", freshBytes()},
        {"Sidebar Cafe", sidebar()},
        {"Sunstream Cafe", sunstream(), thursday},
        {"The Cellar Market", {
            {"Ready-to-go sandwich"}, {"Ready-to-go salad"}, {"Bottled drink"}, {"Microwavable meal"},
            {"Baked treat"}, {"Snack"}, {"Fresh produce"}
        }, general},
        {"The Bronco", {{"Brick-oven pizza"}, {"Burger"}, {"Hot appetizer"}}, general},
        {"Wednesday Food Trucks", {}, "Wednesday service. Enter the dish and points from your receipt."}
    };
    return catalog;
}
}
