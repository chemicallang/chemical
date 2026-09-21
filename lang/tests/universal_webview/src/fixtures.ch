// Components E2E demo app.
//
// One page (index) that renders every interactive component fixture. Each
// fixture is wrapped in an element with a stable `data-testid` so Playwright
// tests can target it regardless of hashed class names. State is owned by the
// fixture so tests can click through SSR -> hydration -> interaction.

// ---------------------------------------------------------------------------
// Counter: the core hydration test. If hydration breaks, clicks either don't
// fire or the count resets to the SSR value.
// ---------------------------------------------------------------------------
#universal CounterFixture(props) {
    state count = 0
    return <div data-testid="counter-fixture">
        <p data-testid="counter-value">Count: {count}</p>
        <Button data-testid="counter-increment" onClick={() => count += 1}>Increment</Button>
        <Button data-testid="counter-reset" variant="ghost" onClick={() => count = 0}>Reset</Button>
    </div>
}

// ---------------------------------------------------------------------------
// Button: all variants, sizes, loading, disabled
// ---------------------------------------------------------------------------
#universal ButtonFixture(props) {
    state loading = false
    return <div data-testid="button-fixture">
        <Button data-testid="btn-default">Default</Button>
        <Button data-testid="btn-destructive" variant="destructive">Destructive</Button>
        <Button data-testid="btn-outline" variant="outline">Outline</Button>
        <Button data-testid="btn-secondary" variant="secondary">Secondary</Button>
        <Button data-testid="btn-ghost" variant="ghost">Ghost</Button>
        <Button data-testid="btn-link" variant="link">Link</Button>
        <Button data-testid="btn-success" variant="success">Success</Button>
        <Button data-testid="btn-warning" variant="warning">Warning</Button>
        <Button data-testid="btn-info" variant="info">Info</Button>
        <Button data-testid="btn-accent" variant="accent">Accent</Button>
        <Button data-testid="btn-sm" size="sm">Small</Button>
        <Button data-testid="btn-lg" size="lg">Large</Button>
        <Button data-testid="btn-icon" size="icon">*</Button>
        <Button data-testid="btn-disabled" disabled={true}>Disabled</Button>
        <Button data-testid="btn-loading" loading={loading} onClick={() => { loading = true; setTimeout(() => loading = false, 500) }}>Load</Button>
        <Button data-testid="btn-click" onClick={() => {}}>Click me</Button>
    </div>
}

// ---------------------------------------------------------------------------
// Tabs: stateful tabs from arrays
// ---------------------------------------------------------------------------
#universal TabsFixture(props) {
    return <div data-testid="tabs-fixture">
        <Tabs tabs={["Alpha", "Beta", "Gamma"]} panels={["Panel A", "Panel B", "Panel C"]} defaultIndex={0} />
    </div>
}

// ---------------------------------------------------------------------------
// Accordion item
// ---------------------------------------------------------------------------
#universal AccordionFixture(props) {
    return <div data-testid="accordion-fixture">
        <Accordion>
            <AccordionItem data-testid="acc-item-0" trigger="What is Chemical?" defaultOpen={false}>A programming language.</AccordionItem>
            <AccordionItem data-testid="acc-item-1" trigger="Is it fast?" defaultOpen={false}>Very fast.</AccordionItem>
            <AccordionItem data-testid="acc-item-2" trigger="Who uses it?" defaultOpen={false}>Everyone.</AccordionItem>
        </Accordion>
    </div>
}

// ---------------------------------------------------------------------------
// Dialog (controlled via open/onClose)
// ---------------------------------------------------------------------------
#universal DialogFixture(props) {
    state open = false
    return <div data-testid="dialog-fixture">
        <Button data-testid="dialog-open" onClick={() => open = true}>Open dialog</Button>
        <Dialog open={open} onClose={() => open = false}>
            <DialogContent data-testid="dialog-content">
                <DialogHeader>
                    <H3>Dialog title</H3>
                </DialogHeader>
                <p>Dialog body text</p>
                <DialogActions>
                    <Button data-testid="dialog-confirm" onClick={() => open = false}>Confirm</Button>
                    <Button variant="ghost" data-testid="dialog-cancel" onClick={() => open = false}>Cancel</Button>
                </DialogActions>
            </DialogContent>
        </Dialog>
    </div>
}

// ---------------------------------------------------------------------------
// Select (custom dropdown, options mode)
// ---------------------------------------------------------------------------
#universal SelectFixture(props) {
    state value = ""
    return <div data-testid="select-fixture">
        <Select data-testid="select-control" options={["Apple", "Banana", "Cherry"]} value={value} onValueChange={(v) => value = v} placeholder="Pick a fruit" />
        <p data-testid="select-value">Chosen: {value ? value : "none"}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Portals: Select menus must escape overflow:hidden / transform clipping
// ---------------------------------------------------------------------------
#universal PortalFixture(props) {
    state value = ""
    return <div data-testid="portal-fixture">
        <div style="overflow:hidden;height:70px;padding:12px;border:1px solid hsl(var(--border));">
            <p style="font-size:0.75rem;color:hsl(var(--muted-foreground));">Overflow hidden container</p>
            <Select data-testid="portal-overflow-select" options={["One", "Two", "Three"]} value={value} onValueChange={(v) => value = v} placeholder="Overflow pick" />
        </div>
        <div style="transform:translateX(0);height:70px;padding:12px;margin-top:1rem;border:1px solid hsl(var(--border));">
            <p style="font-size:0.75rem;color:hsl(var(--muted-foreground));">Transform container</p>
            <Select data-testid="portal-transform-select" options={["Alpha", "Beta", "Gamma"]} value={value} onValueChange={(v) => value = v} placeholder="Transform pick" />
        </div>
        <p data-testid="portal-value">Chosen: {value ? value : "none"}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Slider: click the track, verify aria-valuenow + keyboard
// ---------------------------------------------------------------------------
#universal SliderFixture(props) {
    state value = 30
    return <div data-testid="slider-fixture">
        <Slider data-testid="slider-control" min={0} max={100} step={10} value={value} onValueChange={(v) => value = v} ariaLabel="Volume" />
        <p data-testid="slider-value">Value: {value}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Checkbox / Switch / Radio
// ---------------------------------------------------------------------------
#universal ToggleFixture(props) {
    state checked = false
    state switched = true
    state radio = "a"
    return <div data-testid="toggle-fixture">
        <Checkbox data-testid="checkbox-control" checked={checked} onClick={() => checked = !checked}>Enable</Checkbox>
        <Switch data-testid="switch-control" checked={switched} onClick={() => switched = !switched}>Notify</Switch>
        <Radio data-testid="radio-a" checked={radio == "a"} name="choice" onClick={() => radio = "a"}>Option A</Radio>
        <Radio data-testid="radio-b" checked={radio == "b"} name="choice" onClick={() => radio = "b"}>Option B</Radio>
    </div>
}

// ---------------------------------------------------------------------------
// ToggleGroup (single)
// ---------------------------------------------------------------------------
#universal ToggleGroupFixture(props) {
    return <div data-testid="togglegroup-fixture">
        <ToggleGroup name="tg-main" type="single" defaultValue="Bold" onValueChange={(v) => {}}
            data-testid="tg-group">
            <ToggleGroupItem value="Bold">Bold</ToggleGroupItem>
            <ToggleGroupItem value="Italic">Italic</ToggleGroupItem>
            <ToggleGroupItem value="Underline">Underline</ToggleGroupItem>
        </ToggleGroup>
    </div>
}

// ---------------------------------------------------------------------------
// ToggleGroup (multiple)
// ---------------------------------------------------------------------------
#universal ToggleGroupMultipleFixture(props) {
    return <div data-testid="togglegroup-multi-fixture">
        <ToggleGroup name="tg-multi" type="multiple" defaultValue={["Bold"]}>
            <ToggleGroupItem value="Bold">Bold</ToggleGroupItem>
            <ToggleGroupItem value="Italic">Italic</ToggleGroupItem>
            <ToggleGroupItem value="Underline">Underline</ToggleGroupItem>
        </ToggleGroup>
    </div>
}

// ---------------------------------------------------------------------------
// ToggleGroup scoping: two UNNAMED groups must keep independent selection
// (previously they shared one global context entry keyed "tg-default").
// ---------------------------------------------------------------------------
#universal ToggleGroupScopedFixture(props) {
    return <div data-testid="togglegroup-scoped-fixture">
        <div data-testid="tgs-a">
            <ToggleGroup type="single" defaultValue="Bold">
                <ToggleGroupItem value="Bold">A-Bold</ToggleGroupItem>
                <ToggleGroupItem value="Italic">A-Italic</ToggleGroupItem>
            </ToggleGroup>
        </div>
        <div data-testid="tgs-b">
            <ToggleGroup type="single" defaultValue="Italic">
                <ToggleGroupItem value="Bold">B-Bold</ToggleGroupItem>
                <ToggleGroupItem value="Italic">B-Italic</ToggleGroupItem>
            </ToggleGroup>
        </div>
    </div>
}

// ---------------------------------------------------------------------------
// RadioGroup (children mode)
// ---------------------------------------------------------------------------
#universal RadioGroupFixture(props) {
    return <div data-testid="radiogroup-fixture">
        <RadioGroup name="size" defaultValue="Medium" onValueChange={(v) => {}}
            data-testid="rg-group">
            <RadioGroupItem value="Small">Small</RadioGroupItem>
            <RadioGroupItem value="Medium">Medium</RadioGroupItem>
            <RadioGroupItem value="Large">Large</RadioGroupItem>
        </RadioGroup>
    </div>
}

// ---------------------------------------------------------------------------
// RadioGroup without a provider: items must degrade to the default (unchecked)
// ---------------------------------------------------------------------------
#universal RadioGroupNoProviderFixture(props) {
    return <div data-testid="radiogroup-noprovider-fixture">
        <RadioGroupItem value="Solo">Solo</RadioGroupItem>
    </div>
}

// ---------------------------------------------------------------------------
// Toast: show + auto-dismiss via duration
// ---------------------------------------------------------------------------
#universal ToastFixture(props) {
    state show = true
    return <div data-testid="toast-fixture">
        <ToastViewport data-testid="toast-viewport">
            {show ? <Toast data-testid="toast-item" title="Saved" description="Changes saved" duration={1500} onClose={() => show = false} /> : null}
        </ToastViewport>
    </div>
}

// ---------------------------------------------------------------------------
// Collapsible
// ---------------------------------------------------------------------------
#universal CollapsibleFixture(props) {
    return <div data-testid="collapsible-fixture">
        <Collapsible data-testid="collapsible-control" trigger="More info">Hidden details here.</Collapsible>
    </div>
}

// ---------------------------------------------------------------------------
// Sheet (controlled)
// ---------------------------------------------------------------------------
#universal SheetFixture(props) {
    state open = false
    return <div data-testid="sheet-fixture">
        <Button data-testid="sheet-open" onClick={() => open = true}>Open sheet</Button>
        <Sheet open={open} onClose={() => open = false} title="Settings" side="right">
            <p>Sheet body</p>
        </Sheet>
    </div>
}

// ---------------------------------------------------------------------------
// Dropdown (portaled menu)
// ---------------------------------------------------------------------------
#universal DropdownFixture(props) {
    state open = false
    return <div data-testid="dropdown-fixture">
        <div style="overflow:hidden;height:60px;padding:10px;border:1px solid hsl(var(--border));">
            <Dropdown data-testid="dropdown-control" open={open} onClose={() => open = false} onToggle={() => open = !open} trigger="Actions">
                <DropdownItem onClick={() => open = false}>Rename</DropdownItem>
                <DropdownItem onClick={() => open = false}>Delete</DropdownItem>
            </Dropdown>
        </div>
    </div>
}

// ---------------------------------------------------------------------------
// Error boundary: a component whose render throws is replaced by its
// useErrorBoundary fallback (or the default chx-error-boundary UI) instead of
// taking down the page.
// ---------------------------------------------------------------------------
#universal BadComponent(props) {
    useErrorBoundary(() => <p data-testid="error-fallback" role="alert">Fallback shown</p>)
    var boom = () => { throw new Error("bad component"); }
    boom()
    return <p>never rendered</p>
}

#universal BadComponentDefault(props) {
    var boom = () => { throw new Error("bad component"); }
    boom()
    return <p>never rendered</p>
}

#universal ErrorBoundaryFixture(props) {
    state show = false
    state showDefault = false
    return <div data-testid="error-fixture">
        <Button data-testid="error-mount" onClick={() => show = true}>Mount broken</Button>
        <Button data-testid="error-default-mount" variant="ghost" onClick={() => showDefault = true}>Mount default fallback</Button>
        {show ? <BadComponent /> : null}
        {showDefault ? <BadComponentDefault /> : null}
    </div>
}

// ===========================================================================
// NEW FIXTURES: Alert, Avatar, Badge, Card, Input, Separator, Typography,
//               Progress, Pagination, List, Table, Tooltip
// ===========================================================================

// ---------------------------------------------------------------------------
// Alert: variants, dismissible, title+description
// ---------------------------------------------------------------------------
#universal AlertFixture(props) {
    state dismissed = false
    return <div data-testid="alert-fixture">
        {dismissed ? null : <Alert data-testid="alert-info" variant="info" title="Heads up" description="This is info." dismissible onDismiss={() => dismissed = true} />}
        <Alert data-testid="alert-success" variant="success" title="Done" description="It worked." />
        <Alert data-testid="alert-error" variant="error" title="Oops" description="Something broke." />
        <Alert data-testid="alert-warning" variant="warning" title="Careful" description="Watch out." />
        <Alert data-testid="alert-default" title="Note" description="Default alert." />
        <Alert data-testid="alert-accent" variant="accent" title="Accent" description="Accent style." />
    </div>
}

// ---------------------------------------------------------------------------
// Avatar: sizes, fallback, src, bordered, group
// ---------------------------------------------------------------------------
#universal AvatarFixture(props) {
    return <div data-testid="avatar-fixture">
        <Avatar data-testid="avatar-xs" size="xs" fallback="XS" />
        <Avatar data-testid="avatar-sm" size="sm" fallback="SM" />
        <Avatar data-testid="avatar-md" fallback="MD" />
        <Avatar data-testid="avatar-lg" size="lg" fallback="LG" />
        <Avatar data-testid="avatar-xl" size="xl" fallback="XL" />
        <Avatar data-testid="avatar-bordered" bordered={true} fallback="BD" />
        <AvatarGroup data-testid="avatar-group">
            <Avatar fallback="A" />
            <Avatar fallback="B" />
            <Avatar fallback="C" />
        </AvatarGroup>
        <AvatarMore data-testid="avatar-more" count="+5" />
    </div>
}

// ---------------------------------------------------------------------------
// Badge: variants, sizes
// ---------------------------------------------------------------------------
#universal BadgeFixture(props) {
    return <div data-testid="badge-fixture">
        <Badge data-testid="badge-default">Default</Badge>
        <Badge data-testid="badge-secondary" variant="secondary">Secondary</Badge>
        <Badge data-testid="badge-accent" variant="accent">Accent</Badge>
        <Badge data-testid="badge-success" variant="success">Success</Badge>
        <Badge data-testid="badge-error" variant="error">Error</Badge>
        <Badge data-testid="badge-warning" variant="warning">Warning</Badge>
        <Badge data-testid="badge-info" variant="info">Info</Badge>
        <Badge data-testid="badge-outline" variant="outline">Outline</Badge>
        <Badge data-testid="badge-xs" size="xs">XS</Badge>
        <Badge data-testid="badge-sm" size="sm">SM</Badge>
        <Badge data-testid="badge-lg" size="lg">LG</Badge>
    </div>
}

// ---------------------------------------------------------------------------
// Card: structure (header, title, description, content, footer, action)
// ---------------------------------------------------------------------------
#universal CardFixture(props) {
    state clicked = false
    return <div data-testid="card-fixture">
        <Card data-testid="card-basic">
            <CardHeader>
                <CardTitle>Card title</CardTitle>
                <CardDescription>Card description text.</CardDescription>
            </CardHeader>
            <CardContent>
                <p data-testid="card-content">Card body content.</p>
            </CardContent>
            <CardFooter>
                <Button data-testid="card-action-btn" size="sm">Action</Button>
            </CardFooter>
        </Card>
        <Card data-testid="card-interactive" onClick={() => clicked = true}>
            <CardContent>
                <p data-testid="card-interactive-text">{clicked ? "Clicked!" : "Click me"}</p>
            </CardContent>
        </Card>
        <Card data-testid="card-level2">
            <CardTitle level={2}>H2 title</CardTitle>
        </Card>
        <Card data-testid="card-action-slot">
            <CardHeader>
                <CardAction><Button size="icon">*</Button></CardAction>
                <CardTitle>With action</CardTitle>
            </CardHeader>
        </Card>
    </div>
}

// ---------------------------------------------------------------------------
// Input: variants, sizes, disabled, placeholder, TextArea, Field
// ---------------------------------------------------------------------------
#universal InputFixture(props) {
    state text = ""
    state area = ""
    return <div data-testid="input-fixture">
        <Input data-testid="input-default" placeholder="Default input" value={text} onChange={(e) => text = e.target.value} />
        <Input data-testid="input-filled" variant="filled" placeholder="Filled" />
        <Input data-testid="input-ghost" variant="ghost" placeholder="Ghost" />
        <Input data-testid="input-error" variant="error" placeholder="Error" />
        <Input data-testid="input-success" variant="success" placeholder="Success" />
        <Input data-testid="input-sm" size="sm" placeholder="Small" />
        <Input data-testid="input-lg" size="lg" placeholder="Large" />
        <Input data-testid="input-disabled" disabled={true} placeholder="Disabled" />
        <p data-testid="input-value">{text ? text : "empty"}</p>
        <TextArea data-testid="textarea-default" placeholder="Type here..." value={area} onChange={(e) => area = e.target.value} />
        <Field data-testid="field-default" label="Email" hint="We won't share this.">
            <Input placeholder="you@example.com" />
        </Field>
        <Field data-testid="field-error" label="Password" error="Too short.">
            <Input placeholder="***" />
        </Field>
    </div>
}

// ---------------------------------------------------------------------------
// Separator
// ---------------------------------------------------------------------------
#universal SeparatorFixture(props) {
    return <div data-testid="separator-fixture">
        <p>Above</p>
        <Separator data-testid="separator-h" orientation="horizontal" />
        <p>Below</p>
    </div>
}

// ---------------------------------------------------------------------------
// Typography: headings, text, lead, caption, code, link, blockquote
// ---------------------------------------------------------------------------
#universal TypographyFixture(props) {
    return <div data-testid="typography-fixture">
        <H1 data-testid="ty-h1">Heading 1</H1>
        <H2 data-testid="ty-h2">Heading 2</H2>
        <H3 data-testid="ty-h3">Heading 3</H3>
        <H4 data-testid="ty-h4">Heading 4</H4>
        <H5 data-testid="ty-h5">Heading 5</H5>
        <H6 data-testid="ty-h6">Heading 6</H6>
        <Heading data-testid="ty-heading3" level={3}>Dynamic level</Heading>
        <Text data-testid="ty-text">Paragraph text.</Text>
        <Text data-testid="ty-text-muted" muted={true}>Muted text.</Text>
        <Text data-testid="ty-text-span" as="span">Inline span.</Text>
        <Text data-testid="ty-text-div" as="div">Div text.</Text>
        <Lead data-testid="ty-lead">Lead paragraph.</Lead>
        <Caption data-testid="ty-caption">Caption text.</Caption>
        <CodeText data-testid="ty-code">console.log()</CodeText>
        <Link data-testid="ty-link" href="https://example.com">External link</Link>
        <Blockquote data-testid="ty-blockquote" cite="Someone">A wise quote.</Blockquote>
    </div>
}

// ---------------------------------------------------------------------------
// Progress
// ---------------------------------------------------------------------------
#universal ProgressFixture(props) {
    state value = 45
    return <div data-testid="progress-fixture">
        <Progress data-testid="progress-default" value={value} />
        <Progress data-testid="progress-primary" variant="primary" value={60} />
        <Progress data-testid="progress-success" variant="success" value={80} />
        <Progress data-testid="progress-warning" variant="warning" value={30} />
        <Progress data-testid="progress-error" variant="error" value={90} />
        <Progress data-testid="progress-info" variant="info" value={55} />
        <Progress data-testid="progress-sm" size="sm" value={40} />
        <Progress data-testid="progress-lg" size="lg" value={70} />
        <Button onClick={() => value = value < 100 ? value + 10 : 0}>Change</Button>
    </div>
}

// ---------------------------------------------------------------------------
// Pagination
// ---------------------------------------------------------------------------
#universal PaginationFixture(props) {
    state currentPage = 1
    return <div data-testid="pagination-fixture">
        <Pagination data-testid="pagination" pages={[1, 2, 3, 4, 5]} defaultPage={1} onChange={(p) => currentPage = p} />
        <p data-testid="pagination-value">Page: {currentPage}</p>
    </div>
}

// ---------------------------------------------------------------------------
// List
// ---------------------------------------------------------------------------
#universal ListFixture(props) {
    return <div data-testid="list-fixture">
        <List data-testid="list">
            <ListItem>First item</ListItem>
            <ListItem>Second item</ListItem>
            <ListItem>Third item</ListItem>
        </List>
    </div>
}

// ---------------------------------------------------------------------------
// Table
// ---------------------------------------------------------------------------
#universal TableFixture(props) {
    return <div data-testid="table-fixture">
        <Table data-testid="table">
            <thead>
                <tr>
                    <TableHeadCell>Name</TableHeadCell>
                    <TableHeadCell>Value</TableHeadCell>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <TableCell>Alpha</TableCell>
                    <TableCell>100</TableCell>
                </tr>
                <tr>
                    <TableCell>Beta</TableCell>
                    <TableCell>200</TableCell>
                </tr>
            </tbody>
        </Table>
    </div>
}

// ---------------------------------------------------------------------------
// Tooltip
// ---------------------------------------------------------------------------
#universal TooltipFixture(props) {
    return <div data-testid="tooltip-fixture">
        <Tooltip data-testid="tooltip-top" label="Top tip">
            <Button size="sm">Hover top</Button>
        </Tooltip>
        <Tooltip data-testid="tooltip-bottom" label="Bottom tip" position="bottom">
            <Button size="sm">Hover bottom</Button>
        </Tooltip>
    </div>
}

// ---------------------------------------------------------------------------
// Pagination keyboard navigation
// ---------------------------------------------------------------------------
#universal PaginationKeyboardFixture(props) {
    state currentPage = 3
    return <div data-testid="pagination-keyboard-fixture">
        <Pagination data-testid="pagination-kb" pages={[1, 2, 3, 4, 5]} defaultPage={3} onChange={(p) => currentPage = p} />
        <p data-testid="pagination-kb-value">Page: {currentPage}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Accordion multiple open
// ---------------------------------------------------------------------------
#universal AccordionMultiFixture(props) {
    return <div data-testid="accordion-multi-fixture">
        <Accordion>             <AccordionItem trigger="Item A" defaultOpen={true}>Content A</AccordionItem>
             <AccordionItem trigger="Item B" defaultOpen={true}>Content B</AccordionItem>
             <AccordionItem trigger="Item C" defaultOpen={false}>Content C</AccordionItem>
        </Accordion>
    </div>
}

// ---------------------------------------------------------------------------
// Nested interactive components
// ---------------------------------------------------------------------------
#universal NestedFixture(props) {
    state count = 0
    state text = ""
    return <div data-testid="nested-fixture">
        <Card data-testid="nested-card">
            <CardHeader>
                <CardTitle>Nested test</CardTitle>
            </CardHeader>
            <CardContent>
                <Input data-testid="nested-input" placeholder="Type..." value={text} onChange={(e) => text = e.target.value} />
                <Button data-testid="nested-btn" onClick={() => count += 1}>Add</Button>
                <p data-testid="nested-count">Count: {count}</p>
                <p data-testid="nested-text">{text ? text : "empty"}</p>
            </CardContent>
        </Card>
    </div>
}

// ---------------------------------------------------------------------------
// Performance fixture: render many components to measure SSR + hydration perf
// ---------------------------------------------------------------------------
#universal PerfFixture(props) {
    state count = 0
    return <div data-testid="perf-fixture">
        <Button data-testid="perf-btn" onClick={() => count += 1}>Click</Button>
        <p data-testid="perf-count">{count}</p>
    </div>
}

// ===========================================================================
// EDGE-CASE FIXTURES
// ===========================================================================

// ---------------------------------------------------------------------------
// Button edge cases: type submit, aria-label, Fab, disabled
// ---------------------------------------------------------------------------
#universal ButtonEdgeFixture(props) {
    state submitted = false
    return <div data-testid="button-edge-fixture">
        <Button data-testid="btn-submit" type="submit" onClick={() => submitted = true}>Submit</Button>
        <Button data-testid="btn-aria" aria-label="Close dialog">X</Button>
        <Button data-testid="btn-disabled-interactive" disabled={true} onClick={() => submitted = true}>No click</Button>
        <Fab data-testid="btn-fab" ariaLabel="Add item">+</Fab>
        <Fab data-testid="btn-fab-disabled" disabled={true} ariaLabel="Disabled">X</Fab>
        <p data-testid="btn-submit-state">{submitted ? "submitted" : "not submitted"}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Input edge cases: type variations, aria-label, onChange, NativeSelect, rows
// ---------------------------------------------------------------------------
#universal InputEdgeFixture(props) {
    state email = ""
    state num = ""
    state pw = ""
    state search = ""
    state area = ""
    state selected = ""
    return <div data-testid="input-edge-fixture">
        <Input data-testid="input-email" type="email" placeholder="Email" ariaLabel="Email input" value={email} onChange={(e) => email = e.target.value} />
        <Input data-testid="input-number" type="number" placeholder="0" value={num} onChange={(e) => num = e.target.value} />
        <Input data-testid="input-password" type="password" placeholder="***" value={pw} onChange={(e) => pw = e.target.value} />
        <Input data-testid="input-search" type="search" placeholder="Search..." value={search} onChange={(e) => search = e.target.value} />
        <TextArea data-testid="textarea-rows" rows={5} placeholder="5 rows" value={area} onChange={(e) => area = e.target.value} />
        <NativeSelect data-testid="native-select" value={selected} onChange={(e) => selected = e.target.value} placeholder="Pick...">
            <option value="a">Alpha</option>
            <option value="b">Beta</option>
            <option value="g">Gamma</option>
        </NativeSelect>
        <p data-testid="input-email-value">{email ? email : "empty"}</p>
        <p data-testid="input-num-value">{num ? num : "empty"}</p>
        <p data-testid="input-pw-value">{pw ? pw : "empty"}</p>
        <p data-testid="native-select-value">{selected ? selected : "none"}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Select edge cases: disabled, empty options, controlled, sizes, aria
// ---------------------------------------------------------------------------
#universal SelectEdgeFixture(props) {
    state value = "Apple"
    return <div data-testid="select-edge-fixture">
        <Select data-testid="select-controlled" options={["Apple", "Banana", "Cherry"]} value={value} onValueChange={(v) => value = v} placeholder="Controlled" />
        <Select data-testid="select-disabled" options={["A", "B"]} disabled={true} placeholder="Disabled" />
        <Select data-testid="select-empty" options={[]} placeholder="No options" />
        <Select data-testid="select-sm" options={["S1", "S2"]} size="sm" placeholder="Small" />
        <Select data-testid="select-lg" options={["L1", "L2"]} size="lg" placeholder="Large" />
        <Select data-testid="select-defaultvalue" options={["X", "Y", "Z"]} defaultValue="Y" placeholder="Default Y" />
        <p data-testid="select-controlled-value">{value}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Slider edge cases: disabled, custom range, ArrowUp/Down, controlled
// ---------------------------------------------------------------------------
#universal SliderEdgeFixture(props) {
    state value = 50
    return <div data-testid="slider-edge-fixture">
        <Slider data-testid="slider-disabled" min={0} max={100} value={30} disabled={true} ariaLabel="Disabled slider" />
        <Slider data-testid="slider-custom-range" min={10} max={20} step={1} defaultValue={15} ariaLabel="Custom range" />
        <Slider data-testid="slider-controlled" min={0} max={100} step={5} value={value} onValueChange={(v) => value = v} ariaLabel="Controlled" />
        <Slider data-testid="slider-default" min={0} max={100} step={1} defaultValue={0} ariaLabel="Default zero" />
        <p data-testid="slider-controlled-value">{value}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Toast edge cases: variants, duration=0, manual close, action, role=status
// ---------------------------------------------------------------------------
#universal ToastEdgeFixture(props) {
    state showSuccess = true
    state showDestructive = true
    state showPersistent = true
    return <div data-testid="toast-edge-fixture">
        <ToastViewport data-testid="toast-edge-viewport">
            {showSuccess ? <Toast data-testid="toast-success" title="Success toast" variant="success" duration={800} onClose={() => showSuccess = false} /> : null}
            {showDestructive ? <Toast data-testid="toast-destructive" title="Error toast" variant="destructive" duration={800} onClose={() => showDestructive = false} /> : null}
            {showPersistent ? <Toast data-testid="toast-persistent" title="Persistent" description="No auto-dismiss" duration={0} action="Undo" actionClick={() => showPersistent = false} onClose={() => showPersistent = false} /> : null}
        </ToastViewport>
    </div>
}

// ---------------------------------------------------------------------------
// Toggle edge cases: disabled, sizes, data-disabled, aria-label
// ---------------------------------------------------------------------------
#universal ToggleEdgeFixture(props) {
    state cb = false
    state sw = true
    return <div data-testid="toggle-edge-fixture">
        <Checkbox data-testid="cb-disabled" disabled={true}>Disabled CB</Checkbox>
        <Checkbox data-testid="cb-sm" size="sm">Small CB</Checkbox>
        <Checkbox data-testid="cb-lg" size="lg">Large CB</Checkbox>
        <Checkbox data-testid="cb-aria" ariaLabel="Accept terms">Aria CB</Checkbox>
        <Switch data-testid="sw-disabled" disabled={true}>Disabled SW</Switch>
        <Switch data-testid="sw-sm" size="sm">Small SW</Switch>
        <Switch data-testid="sw-lg" size="lg">Large SW</Switch>
        <Radio data-testid="radio-disabled" disabled={true} name="edge-radio">Disabled Radio</Radio>
        <Radio data-testid="radio-sm" size="sm" name="edge-radio-2">Small Radio</Radio>
        <Radio data-testid="radio-lg" size="lg" name="edge-radio-2">Large Radio</Radio>
        <p data-testid="toggle-edge-cb">{cb ? "on" : "off"}</p>
        <p data-testid="toggle-edge-sw">{sw ? "on" : "off"}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Collapsible edge cases: disabled, defaultOpen=true, onOpenChange
// ---------------------------------------------------------------------------
#universal CollapsibleEdgeFixture(props) {
    state lastState = "closed"
    return <div data-testid="collapsible-edge-fixture">
        <Collapsible data-testid="collapsible-defaultopen" trigger="Default open" defaultOpen={true}>Always visible content.</Collapsible>
        <Collapsible data-testid="collapsible-disabled" trigger="Disabled" disabled={true}>Hidden content.</Collapsible>
        <Collapsible data-testid="collapsible-callback" trigger="With callback" onOpenChange={(o) => lastState = o ? "open" : "closed"}>Callback content.</Collapsible>
        <p data-testid="collapsible-callback-state">{lastState}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Dialog edge cases: backdrop click, uncontrolled defaultOpen, aria-label
// ---------------------------------------------------------------------------
#universal DialogEdgeFixture(props) {
    state open = false
    return <div data-testid="dialog-edge-fixture">
        <Button data-testid="dialog-edge-open" onClick={() => open = true}>Open</Button>
        <Dialog data-testid="dialog-edge-controlled" open={open} onClose={() => open = false} ariaLabel="Edge dialog">
            <DialogContent data-testid="dialog-edge-content">
                <p>Edge dialog content</p>
                <Button data-testid="dialog-edge-close" onClick={() => open = false}>Close</Button>
            </DialogContent>
        </Dialog>
        <p data-testid="dialog-edge-state">{open ? "open" : "closed"}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Accordion edge cases: disabled items, subtitle, custom chevrons
// ---------------------------------------------------------------------------
#universal AccordionEdgeFixture(props) {
    return <div data-testid="accordion-edge-fixture">
        <Accordion>
            <AccordionItem data-testid="acc-disabled" trigger="Disabled item" disabled={true}>Hidden.</AccordionItem>
            <AccordionItem data-testid="acc-subtitle" trigger="With subtitle" defaultOpen={false}>Content.</AccordionItem>
            <AccordionItem data-testid="acc-custom-chevron" trigger="Custom chevrons" defaultOpen={false}>Custom chevrons.</AccordionItem>
        </Accordion>
    </div>
}

// ---------------------------------------------------------------------------
// Tabs edge cases: onChange, ariaLabel
// ---------------------------------------------------------------------------
#universal TabsEdgeFixture(props) {
    state lastTab = 0
    return <div data-testid="tabs-edge-fixture">
        <Tabs tabs={["One", "Two"]} panels={["Panel 1", "Panel 2"]} defaultIndex={0} onChange={(i) => lastTab = i} ariaLabel="Edge tabs" />
        <p data-testid="tabs-edge-last">{lastTab}</p>
    </div>
}

// ---------------------------------------------------------------------------
// Pagination edge cases: last page disables next, first page disables prev
// ---------------------------------------------------------------------------
#universal PaginationEdgeFixture(props) {
    state currentPage = 5
    return <div data-testid="pagination-edge-fixture">
        <Pagination data-testid="pagination-edge-last" pages={[1, 2, 3, 4, 5]} defaultPage={5} onChange={(p) => currentPage = p} />
        <p data-testid="pagination-edge-value">{currentPage}</p>
    </div>
}

// ---------------------------------------------------------------------------
// RadioGroup keyboard navigation
// ---------------------------------------------------------------------------
#universal RadioGroupKeyboardFixture(props) {
    return <div data-testid="radiogroup-keyboard-fixture">
        <RadioGroup name="kb-size" defaultValue="Medium">
            <RadioGroupItem value="Small">Small</RadioGroupItem>
            <RadioGroupItem value="Medium">Medium</RadioGroupItem>
            <RadioGroupItem value="Large">Large</RadioGroupItem>
        </RadioGroup>
    </div>
}

// ---------------------------------------------------------------------------
// ToggleGroup disabled items
// ---------------------------------------------------------------------------
#universal ToggleGroupDisabledFixture(props) {
    return <div data-testid="togglegroup-disabled-fixture">
        <ToggleGroup name="tg-disabled" type="single" defaultValue="Bold">
            <ToggleGroupItem value="Bold">Bold</ToggleGroupItem>
            <ToggleGroupItem value="Italic">Italic</ToggleGroupItem>
            <ToggleGroupItem value="Underline">Underline</ToggleGroupItem>
        </ToggleGroup>
    </div>
}

// ---------------------------------------------------------------------------
// Utility/Layout components
// ---------------------------------------------------------------------------
#universal ContainerFixture(props) {
    return <div data-testid="container-fixture">
        <Container size="sm"><p data-testid="container-sm">SM content</p></Container>
        <Container size="md"><p data-testid="container-md">MD content</p></Container>
        <Container><p data-testid="container-default">Default content</p></Container>
        <Container size="full"><p data-testid="container-full">Full content</p></Container>
    </div>
}

#universal StackFixture(props) {
    return <div data-testid="stack-fixture">
        <Stack direction="row" gap="sm" data-testid="stack-row">
            <span>Row A</span><span>Row B</span><span>Row C</span>
        </Stack>
        <Stack direction="column" gap="lg" data-testid="stack-column">
            <span>Col A</span><span>Col B</span>
        </Stack>
        <Stack direction="row" align="center" justify="between" data-testid="stack-between">
            <span>Left</span><span>Right</span>
        </Stack>
    </div>
}

#universal GridFixture(props) {
    return <div data-testid="grid-fixture">
        <Grid cols="3" gap="sm" data-testid="grid-3">
            <span>Cell 1</span><span>Cell 2</span><span>Cell 3</span>
        </Grid>
    </div>
}

#universal BreadcrumbsFixture(props) {
    return <div data-testid="breadcrumbs-fixture">
        <Breadcrumbs>
            <BreadcrumbItem><BreadcrumbLink href="/">Home</BreadcrumbLink></BreadcrumbItem>
            <BreadcrumbSeparator />
            <BreadcrumbItem><BreadcrumbLink href="/docs">Docs</BreadcrumbLink></BreadcrumbItem>
            <BreadcrumbSeparator separator="→" />
            <BreadcrumbItem><BreadcrumbCurrent>Components</BreadcrumbCurrent></BreadcrumbItem>
        </Breadcrumbs>
    </div>
}

#universal DividerFixture(props) {
    return <div data-testid="divider-fixture">
        <p>Above</p>
        <Divider />
        <p>Below</p>
    </div>
}

#universal KbdFixture(props) {
    return <div data-testid="kbd-fixture">
        <Kbd data-testid="kbd-ctrl">Ctrl</Kbd>
        <Kbd data-testid="kbd-shift">Shift</Kbd>
    </div>
}

#universal SkeletonFixture(props) {
    return <div data-testid="skeleton-fixture">
        <Skeleton data-testid="skeleton-rect" width="200px" height="20px" />
        <Skeleton data-testid="skeleton-circle" circle />
        <Skeleton data-testid="skeleton-default" />
    </div>
}

#universal SpinnerFixture(props) {
    return <div data-testid="spinner-fixture">
        <Spinner data-testid="spinner-sm" size="sm" label="Loading data" />
        <Spinner data-testid="spinner-md" />
        <Spinner data-testid="spinner-lg" size="lg" />
    </div>
}

// ---------------------------------------------------------------------------
// Surface components
// ---------------------------------------------------------------------------
#universal PaperFixture(props) {
    return <div data-testid="paper-fixture">
        <Paper data-testid="paper"><p>Paper content</p></Paper>
    </div>
}

#universal AppBarFixture(props) {
    return <div data-testid="appbar-fixture">
        <AppBar data-testid="appbar"><span>Logo</span><span>Nav</span></AppBar>
    </div>
}

#universal DrawerFixture(props) {
    return <div data-testid="drawer-fixture">
        <Drawer data-testid="drawer"><p>Drawer content</p></Drawer>
    </div>
}

#universal SnackbarFixture(props) {
    return <div data-testid="snackbar-fixture">
        <Snackbar data-testid="snackbar"><span>Saved!</span></Snackbar>
    </div>
}

#universal IconFixture(props) {
    return <div data-testid="icon-fixture">
        <Icon data-testid="icon">A</Icon>
    </div>
}

#universal BottomBarFixture(props) {
    return <div data-testid="bottombar-fixture">
        <BottomBar data-testid="bottombar"><span>Left</span><span>Right</span></BottomBar>
    </div>
}

#universal EmptyStateFixture(props) {
    return <div data-testid="emptystate-fixture">
        <EmptyState data-testid="emptystate"><p>No data found</p></EmptyState>
    </div>
}

#universal StatCardFixture(props) {
    return <div data-testid="statcard-fixture">
        <StatCard data-testid="statcard"><p>Revenue</p><p>$10,000</p></StatCard>
    </div>
}

// ---------------------------------------------------------------------------
// Missing parameter fixtures
// ---------------------------------------------------------------------------
#universal ToastParamsFixture(props) {
    state show = true
    return <div data-testid="toast-params-fixture">
        <ToastViewport />
        <Toast data-testid="toast-nodismiss" duration={0} title="Persistent" description="Should not auto-dismiss" />
        <Toast data-testid="toast-action" title="Action toast" description="Has action" action="Undo" actionClick={() => {}} />
        <Toast data-testid="toast-withtitle" title="Title only" />
        <Toast data-testid="toast-withdesc" description="Description only" />
    </div>
}

#universal SheetSideFixture(props) {
    state openSide = ""
    var openSheet = (side) => { openSide = side }
    var closeSheet = () => { openSide = "" }
    return <div data-testid="sheet-side-fixture">
        <Button data-testid="sheet-open-left" onClick={() => openSheet("left")}>Open Left</Button>
        <Button data-testid="sheet-open-top" onClick={() => openSheet("top")}>Open Top</Button>
        <Button data-testid="sheet-open-bottom" onClick={() => openSheet("bottom")}>Open Bottom</Button>
        <Sheet open={openSide == "left"} onClose={closeSheet} side="left" title="Left Sheet"><p>Left content</p></Sheet>
        <Sheet open={openSide == "top"} onClose={closeSheet} side="top" title="Top Sheet"><p>Top content</p></Sheet>
        <Sheet open={openSide == "bottom"} onClose={closeSheet} side="bottom" title="Bottom Sheet"><p>Bottom content</p></Sheet>
    </div>
}

#universal DialogDefaultOpenFixture(props) {
    state isOpen = false
    return <div data-testid="dialog-defaultopen-fixture">
        <Button data-testid="dialog-auto-open" onClick={() => { isOpen = true }}>Open auto dialog</Button>
        <Dialog open={isOpen} onClose={() => { isOpen = false }} ariaLabel="Auto-open dialog">
            <p>I open when clicked</p>
            <button data-testid="dialog-auto-close">Close</button>
        </Dialog>
    </div>
}

#universal ToggleGroupControlledFixture(props) {
    state value = "Bold"
    return <div data-testid="togglegroup-controlled-fixture">
        <ToggleGroup name="tg-ctrl" type="single" value={value} onValueChange={(v) => { value = v }}>
            <ToggleGroupItem value="Bold">Bold</ToggleGroupItem>
            <ToggleGroupItem value="Italic">Italic</ToggleGroupItem>
        </ToggleGroup>
        <p data-testid="tg-ctrl-value">{value}</p>
    </div>
}

#universal RadioGroupControlledFixture(props) {
    state value = "small"
    return <div data-testid="radiogroup-controlled-fixture">
        <RadioGroup name="rg-ctrl" value={value} onValueChange={(v) => { value = v }}>
            <RadioGroupItem value="small">Small</RadioGroupItem>
            <RadioGroupItem value="large">Large</RadioGroupItem>
        </RadioGroup>
        <p data-testid="rg-ctrl-value">{value}</p>
    </div>
}

#universal CollapsibleControlledFixture(props) {
    state open = false
    return <div data-testid="collapsible-controlled-fixture">
        <Collapsible open={open} onOpenChange={(v) => { open = v }} trigger="Toggle me">
            <p>Controlled content</p>
        </Collapsible>
        <Button data-testid="collapsible-ctrl-toggle" onClick={() => { open = !open }}>{open ? "Close" : "Open"}</Button>
        <p data-testid="collapsible-ctrl-state">{open ? "open" : "closed"}</p>
    </div>
}

#universal ToggleSizesFixture(props) {
    return <div data-testid="toggle-sizes-fixture">
        <Checkbox data-testid="cb-sm" size="sm">Small CB</Checkbox>
        <Checkbox data-testid="cb-md">Default CB</Checkbox>
        <Checkbox data-testid="cb-lg" size="lg">Large CB</Checkbox>
        <Switch data-testid="sw-sm" size="sm">Small SW</Switch>
        <Switch data-testid="sw-md">Default SW</Switch>
        <Switch data-testid="sw-lg" size="lg">Large SW</Switch>
    </div>
}

#universal ToggleGroupVariantsFixture(props) {
    return <div data-testid="togglegroup-variants-fixture">
        <ToggleGroup name="tg-outline" type="single" defaultValue="A" variant="outline">
            <ToggleGroupItem value="A" data-testid="tgi-outline-a">A</ToggleGroupItem>
            <ToggleGroupItem value="B" data-testid="tgi-outline-b">B</ToggleGroupItem>
        </ToggleGroup>
        <ToggleGroup name="tg-ghost" type="single" defaultValue="X" variant="ghost">
            <ToggleGroupItem value="X" data-testid="tgi-ghost-x">X</ToggleGroupItem>
            <ToggleGroupItem value="Y" data-testid="tgi-ghost-y">Y</ToggleGroupItem>
        </ToggleGroup>
    </div>
}

#universal TextPolymorphicFixture(props) {
    return <div data-testid="text-polymorphic-fixture">
        <Text data-testid="text-p" muted>Paragraph</Text>
        <Text data-testid="text-span" as="span" muted>Span text</Text>
        <Text data-testid="text-div" as="div">Div text</Text>
    </div>
}

#universal DarkModeFixture(props) {
    return <div data-testid="darkmode-fixture">
        <div data-testid="darkmode-card" style="background:hsl(var(--card));color:hsl(var(--card-foreground));border:1px solid hsl(var(--border));padding:1rem;">
            <p data-testid="darkmode-text">Theme content</p>
        </div>
    </div>
}

// ===========================================================================
// RUNTIME FIXTURES: Batching & Unmount Cleanup
// ===========================================================================

// ---------------------------------------------------------------------------
// Batching: multiple state updates in a single event handler must be
// coalesced into one synchronous re-render (via automatic batching).
// ---------------------------------------------------------------------------
#universal BatchingFixture(props) {
    state a = 0
    state b = 0
    state c = 0
    return <div data-testid="batching-fixture">
        <p data-testid="batch-a">{a}</p>
        <p data-testid="batch-b">{b}</p>
        <p data-testid="batch-c">{c}</p>
        <Button data-testid="batch-update" onClick={() => { a = 10; b = 20; c = 30 }}>Update All</Button>
    </div>
}

// ---------------------------------------------------------------------------
// Unmount cleanup: when a child component is removed from the DOM, its
// useEffect cleanup functions must run (owner tree disposal).
// ---------------------------------------------------------------------------
#universal UnmountChild(props) {
    useEffect(() => {
        window.__childMounted = true
        return () => { window.__cleanupRan = true }
    }, [])
    return <div data-testid="unmount-child">Child visible</div>
}

#universal UnmountCleanupFixture(props) {
    state show = true
    return <div data-testid="unmount-fixture">
        <Button data-testid="unmount-toggle" onClick={() => show = !show}>{show ? "Hide" : "Show"}</Button>
        <p data-testid="unmount-showing">{show ? "yes" : "no"}</p>
        {show ? <UnmountChild /> : null}
    </div>
}

// ---------------------------------------------------------------------------
// Effect dependency semantics: an effect must re-run only when one of its deps
// actually changes, never on unrelated instance state updates.
// ---------------------------------------------------------------------------
#universal EffectDepsFixture(props) {
    state count = 0
    state unrelated = 0
    state effectRuns = 0
    useEffect(() => {
        effectRuns = effectRuns + 1
    }, [count])
    return <div data-testid="effect-deps-fixture">
        <p data-testid="ed-count">{count}</p>
        <p data-testid="ed-unrelated">{unrelated}</p>
        <p data-testid="ed-runs">{effectRuns}</p>
        <button data-testid="ed-inc-count" onClick={() => count = count + 1}>inc count</button>
        <button data-testid="ed-inc-unrelated" onClick={() => unrelated = unrelated + 1}>inc unrelated</button>
    </div>
}

// Probe: derived (filtered) list recomputed from state. This is the mechanism
// DataTable/Combobox/forms need: a reactive derived array.
#universal DerivedListProbe(props) {
    state query = ""
    var items = ["Apple", "Banana", "Cherry"]
    var filtered = items.filter((it) => it.toLowerCase().includes(query.toLowerCase()))
    return <div data-testid="derived-list-probe">
        <input data-testid="probe-input" value={query} onInput={(e) => query = e.target.value} />
        <ul data-testid="probe-list">
            {filtered.map((it) => <li data-testid={"probe-" + it}>{it}</li>)}
        </ul>
        <p data-testid="probe-count">{filtered.length}</p>
    </div>
}

// Probe: derived array from PROPS (parent passes state signals).
#universal PropsDerivedChild(props) {
    var filtered = props.items.filter((it) => it.text.includes(props.query))
    return <ul data-testid="props-derived-list">
        {filtered.map((it) => <li data-testid={"pdl-" + it.id}>{it.text}</li>)}
    </ul>
}

#universal PropsDerivedFixture(props) {
    state query = ""
    state items = [{id: "a", text: "Apple"}, {id: "b", text: "Banana"}]
    return <div data-testid="props-derived-fixture">
        <PropsDerivedChild items={items} query={query} />
        <input data-testid="pdl-input" value={query} onInput={(e) => query = e.target.value} />
        <button data-testid="pdl-add" onClick={() => items = [...items, {id: "c", text: "Cherry"}]}>add</button>
    </div>
}

#universal KeyedListFixture(props) {
    state items = [{id: "a", label: "Alpha"}, {id: "b", label: "Beta"}, {id: "c", label: "Gamma"}]
    return <div data-testid="keyed-list-fixture">
        <ul data-testid="keyed-list">
            {items.map(item => <li key={item.id} data-testid={"item-" + item.id}>{item.label}</li>)}
        </ul>
        <Button data-testid="keyed-add-delta" onClick={() => {
            items = [...items, {id: "d", label: "Delta"}]
        }}>Add Delta</Button>
        <Button data-testid="keyed-add-alpha-first" onClick={() => {
            items = [{id: "z", label: "Zeta"}, ...items]
        }}>Add Zeta First</Button>
        <Button data-testid="keyed-remove-b" onClick={() => {
            items = items.filter(item => item.id != "b")
        }}>Remove Beta</Button>
        <Button data-testid="keyed-reverse" onClick={() => {
            items = [...items].reverse()
        }}>Reverse</Button>
        <Button data-testid="keyed-replace-all" onClick={() => {
            items = [{id: "x", label: "X-ray"}, {id: "y", label: "Yankee"}]
        }}>Replace All</Button>
    </div>
}

// Keyed list of components with internal state: a reorder must move each item's
// DOM (and keep its mounted state) rather than rebuilding it. Also exercises
// component props inside `.map()` being resolved during SSR.
#universal KeyedComponentRow(props) {
    state hits = 0
    return <li data-testid={"crow-" + props.id}>
        <button data-testid={"crow-inc-" + props.id} onClick={() => { hits = hits + 1 }}>+</button>
        <span data-testid={"crow-hits-" + props.id}>{hits}</span>
        <span data-testid={"crow-label-" + props.id}>{props.label}</span>
    </li>
}

#universal KeyedComponentListFixture(props) {
    state items = [{id: "a", label: "Alpha"}, {id: "b", label: "Beta"}, {id: "c", label: "Gamma"}]
    return <div data-testid="keyed-comp-fixture">
        <ul data-testid="keyed-comp-list">
            {items.map(item => <KeyedComponentRow key={item.id} id={item.id} label={item.label} />)}
        </ul>
        <Button data-testid="keyed-comp-reverse" onClick={() => { items = [...items].reverse() }}>Reverse</Button>
        <Button data-testid="keyed-comp-relabel-a" onClick={() => {
            items = items.map(it => it.id == "a" ? {id: "a", label: "Alpha2"} : it)
        }}>Relabel A</Button>
    </div>
}

#universal ErrorBoundaryChildFixture(props) {
    useErrorBoundary((p, err) => {
        return <div data-testid="parent-fallback">Parent caught: {err.message}</div>
    })
    return <div data-testid="eb-parent">
        <ErrorBoundaryThrowingChild />
        <span data-testid="eb-parent-sibling"> sibling content</span>
    </div>
}

#universal ErrorBoundaryThrowingChild(props) {
    var x = null
    x.field
    return <span>should not render</span>
}

// ---------------------------------------------------------------------------
// Memoization: useMemo, useCallback, $__uni_memo
// ---------------------------------------------------------------------------
#universal MemoExpensiveComp(props) {
    state renders = 0
    renders = renders + 1
    return <div data-testid="memo-expensive-renders">{renders}</div>
}

#universal MemoFixture(props) {
    state counter = 0
    state label = "hello"
    let renderCount = 0
    let rerenderCount = 0
    return <div data-testid="memo-fixture">
        <div data-testid="memo-counter">{counter}</div>
        <div data-testid="memo-label">{label}</div>
        <MemoExpensiveComp data-testid="memo-nocache" label={label} />
        <button data-testid="memo-inc" onClick={() => counter = counter + 1}>inc</button>
        <button data-testid="memo-changelabel" onClick={() => label = label + "!"}>change</button>
    </div>
}

// ---------------------------------------------------------------------------
// SVG namespace: elements render in SVG namespace
// ---------------------------------------------------------------------------
#universal SvgFixture(props) {
    return <div data-testid="svg-fixture">
        <svg data-testid="svg-root" width="100" height="100" viewBox="0 0 100 100">
            <circle data-testid="svg-circle" cx="50" cy="50" r="40" fill="red" />
            <rect data-testid="svg-rect" x="10" y="10" width="30" height="30" fill="blue" />
        </svg>
        <p data-testid="svg-text">SVG above</p>
    </div>
}

// ---------------------------------------------------------------------------
// Ref forwarding: ref prop forwarded to root DOM element
// ---------------------------------------------------------------------------
#universal RefChildComp(props) {
    return <div data-testid="ref-child-root">child content</div>
}

#universal RefForwardingFixture(props) {
    state capturedTag = "none"
    state capturedTestId = "none"
    return <div data-testid="ref-fixture">
        <RefChildComp data-testid="ref-target" ref={(el) => {
            capturedTag = el ? el.tagName.toLowerCase() : "null"
            capturedTestId = el ? el.getAttribute("data-testid") : "null"
        }} />
        <div data-testid="ref-captured-tag">{capturedTag}</div>
        <div data-testid="ref-captured-testid">{capturedTestId}</div>
    </div>
}

// ---------------------------------------------------------------------------
// Nested component root shapes: a fragment (multi-node) root. Exercises
// hydration adoption when a nested component does not render one root element.
// ---------------------------------------------------------------------------
#universal FragmentRootComp(props) {
    return <>
        <span data-testid="frag-one">one</span>
        <span data-testid="frag-two">two</span>
    </>
}

#universal RootShapesFixture(props) {
    state showFrag = false
    return <div data-testid="root-shapes-fixture">
        <FragmentRootComp />
        <span data-testid="root-shapes-next">next</span>
        <Button data-testid="root-shapes-toggle" onClick={() => showFrag = true}>show</Button>
        {showFrag ? <FragmentRootComp /> : null}
    </div>
}

// Host for directly-passed static children (exercises the html_cbi client-vnode
// child path; no SSR HTML should be transported through JS for these children).
#universal StaticChildrenHost(props) {
    return <section data-testid="static-children-host">{props.children}</section>
}

// Async data + Suspense boundary. The load resolves after a short delay; SSR
// renders the fallback (initial loading state), and the client swaps to the
// content once the timeout fires.
#universal SuspenseFixture(props) {
    state loading = true
    state data = ""
    useEffect(() => {
        setTimeout(() => {
            data = "Loaded data"
            loading = false
        }, 40)
    }, [])
    return <div data-testid="suspense-fixture">
        <Suspense loading={loading} fallback="Loading…">
            <span data-testid="suspense-content">{data}</span>
        </Suspense>
    </div>
}

