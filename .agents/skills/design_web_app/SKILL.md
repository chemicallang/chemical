---
name: Designing Web Apps in Chemical
description:
    How complex and scalable web apps are designed in chemical using macro plugins
---

Writing scalable and complex web apps require that you have great organization, components that handle their responsibilities
and the glue that holds everything together is small, predictable and reliable.

### Beginning the app

First you must ask yourself, whether the app is a static app (static pages website like a blog), a SPA (single page application) or 
a server served app (where server serves pages, modifying the actual content based on each user)

Everything happens with the `page` library, which you should import in the `chemical.mod`

#### Writing a single static page with styles

Here's simple functions that execute on the server, to create a single page with its assets into the current directory.

```chemical
func CreateOneSimplePage() {
    // comes from the page library
    var page = HtmlPage()
    
    // lets first put static content in the page
    PutStaticContentInSimplePage(page)
    
    // write the static page to the directory
    // you should definitely analyze the page library (lang/libs/page) for this function's definition
    // so you can understand how it emits the assets for the page
    // index.html + any assets it requires (js / css) will be written to current directory
    page.writeToDirectory("./", "index.html")
}

// this function returns a random class name for the styles we wrote
// the random class name would be like .h23unfi3
func style_button(page : &mut HtmlPage) : *char {
    return #css {
        color : blue;
        padding : 8px;
        border-radius : 4px;
        background : white;
        // targeting a class name present in the entire page
        .my-global-button {
            color : red;        
        }
    }
}
// when we open a {} for the attribute value, it means we are going to write chemical code inside those braces
// NOT JS, just chemical code that would execute on the server 
func PutStaticContentInSimplePage(page : &mut HtmlPage) {
    #js {
        const myElem = document.getElementById("clickable-btn")
        myElem.onclick = () => {
            console.log("you clicked on the button");
        }
    }
    #html {
        <div>
            <button class={style_button(page)}>Hello World</button>
            <button id="clickable-btn" class="my-global-button">This is targeted using global selector</button>
        </div>
    }
}
```

##### Benefits of this approach

- #css is parsed during compile time, class name is computed during compile time
- #html only emits static html, #html doesn't allow any JS
- Guarantees that emitted page is static

##### Limitations of this approach

- Hard to write JS
  - select element, have event listener, modify text (all of a simple state)
  - lambdas can't be given to onClick attribute of element (like React supports)
- Everything calculated at compile time or in the server, client side requires that manually handling events and DOM manipulation
- Components take server side props (because they are native functions)

#### Cannot split #html

You must not do this, an element that begins, must end in the same macro

```chemical
func InvalidHtml(page : &mut HtmlPage) {
    #html {
        <div>
    }
    
    // chemical statements in between
    if(true) {}
    
    #html {
      </div>
    }
}
```
Again do NOT write ^ such code.

Here's how to write correct code.

```chemical
func InvalidHtml(page : &mut HtmlPage) {
    #html {
        <div>
        @{if(true) {
            // i can write chemical code here
        }}
        </div>
    }
}
```

#### Components

To better provide support for JS handling of elements, applying event listeners and DOM manipulation, universal components come into play.

What if instead of server side functions, we wrote components that can be used at client side or server side ?

```chemical
// this function returns a random class name for the styles we wrote
// the random class name would be like .h23unfi3
func style_button(page : &mut HtmlPage) : *char {
    return #css {
        color : blue;
        padding : 8px;
        border-radius : 4px;
        background : white;
        // targeting a class name present in the entire page
        .my-global-button {
            color : red;        
        }
    }
}
#universal MyButton(props) {
    // page is always available inside a universal component
    var server_value = ${style_button(page)}
    state counter = 0
    var arr = ["first", "second"]
    // the .map on arr would break ssr, meaning it would be rendered at client side
    // everything else would render using ssr + hydration
    return <div>
        <button onClick={() => {
            console.log("you pressed my button")
            counter++
        }}>{counter} : {props.text}</button>
        arr.map((i) => {
            <span>{i}</span>
        })
    </div>
}
func PutStaticContentInSimplePage(page : &mut HtmlPage) {
    // you can mount universal components in #html or other #universal components
    // remember in #html, you can't pass js props, if you want to do that, wrap in a universal component
    #html {
        <div>
            <MyButton text="This is my button" />
            <MyButton text="This is my button" />
        </div>
    }
}
```

in `universal` macro, some things are different

- braces `{}` do not mean a server side (chemical) value, braces contain JS expressions
- dollar braces `${}` contain server side chemical values

#### Benefits of using #universal components

- Automatic hydration support
- Lambdas in attributes support (like onClick)
- Supports state for reactive expressions
- Easily manipulate DOM using JS expressions

#### Drawbacks of using #universal components

- No guarantee of static content being generated, something can break SSR + hydration (like .map)
- Not explicitly clear about what gets generated for the runtime code