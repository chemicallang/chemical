
@test
public func keyframes_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        @keyframes slidein {
            from {
                margin-left: 100%;
                width: 300%;
            }
            to {
                margin-left: 0%;
                width: 100%;
            }
        }
    }
    const expected = std::string_view("@keyframes slidein { from { margin-left:100%;width:300%; } to { margin-left:0%;width:100%; } }");
    css_at_rule_equals(env, page.toStringCssOnly(), &expected);
}

@test
public func keyframes_with_percentages_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #css {
        @keyframes pulse {
            0% { transform: scale(1); }
            50% { transform: scale(1.1); }
            100% { transform: scale(1); }
        }
    }
    const expected = std::string_view("@keyframes pulse { 0% { transform:scale(1); } 50% { transform:scale(1.1); } 100% { transform:scale(1); } }");
    css_at_rule_equals(env, page.toStringCssOnly(), &expected);
}
