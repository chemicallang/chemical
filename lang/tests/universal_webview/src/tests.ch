// #universal_test declarations (see main.ch for the components and the runner).

#universal_test("ssr renders initial state") {
    <Counter start={0} />
    <script>
        expect($('[data-testid=count]').text()).toBe('Count: 0')
    </script>
}

#universal_test("props survive SSR") {
    <Greeting name="World" />
    <script>
        expect(byTestId('greet').text()).toBe('Hello World')
    </script>
}

#universal_test("click updates state") {
    <Counter start={5} />
    <script>
        expect($('[data-testid=count]').text()).toBe('Count: 5')
        $('[data-testid=inc]').click()
        expect($('[data-testid=count]').text()).toBe('Count: 6')
    </script>
}

#universal_test("conditional children toggle") {
    <Toggle />
    <script>
        expect(byTestId('panel').exists()).toBe(false)
        byTestId('toggle').click()
        expect(byTestId('panel').text()).toBe('Open')
    </script>
}

#universal_test("typing updates state") {
    <InputBox />
    <script>
        byTestId('input').type('hello')
        expect(byTestId('mirror').text()).toBe('hello')
        expect(byTestId('input').value()).toBe('hello')
    </script>
}

#universal_test("portals/globals isolated", isolate) {
    <Counter start={1} />
    <script>
        expect($('[data-testid=count]').text()).toBe('Count: 1')
        $('[data-testid=inc]').click()
        expect($('[data-testid=count]').text()).toBe('Count: 2')
    </script>
}
