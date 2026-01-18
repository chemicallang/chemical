#component Empty(props) {}

#component TestComp(props) {
    <div>
        <span name={"test"} />
    </div>
}

@test
public func component_attribute_expression_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <TestComp />
    }
    var js = page.toStringHeadJsOnly()
}

#component LambdaComp(props) {
    <div>
        <Empty elem={(p) => { <span /> }} />
    </div>
}

@test
public func component_lambda_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <TestComp />
    }
    var js = page.toStringHeadJsOnly()

}

#component SpreadComp(props) {
    <Empty {...props} />
}

@test
public func component_spread_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #html {
        <SpreadComp />
    }
    var js = page.toStringHeadJsOnly()
}
