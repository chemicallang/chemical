function universal_fixture_Counter(props) { const n = $_us(0); return $_ur.createElement("button", {"class": "counter", "onClick": () => { n.value = n.value + 1; }}, n); }
function universal_fixture_Badge(props) { return $_ur.createElement("span", {"class": "badge"}, window.$__uni_value(props.children)); }
window.$__uni_c(1,'universal_fixture_Counter',{});
window.$__uni_c(2,'universal_fixture_Counter',{});
window.$__uni_c(3,'universal_fixture_Counter',{});
window.$__uni_c(4,'universal_fixture_Badge',{"children":["one"]});
window.$__uni_c(5,'universal_fixture_Badge',{"children":["two"]});
