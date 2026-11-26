import { defineConfig } from '@playwright/test';

const PORT = Number(process.env.WASM_UI_PORT || 4199);

export default defineConfig({
    testDir: '.',
    testMatch: /.*\.spec\.ts/,
    fullyParallel: false,
    workers: 1,
    timeout: 30_000,
    expect: { timeout: 5_000 },
    reporter: [['list']],
    use: {
        baseURL: `http://127.0.0.1:${PORT}`,
        browserName: 'chromium',
        trace: 'retain-on-failure',
        screenshot: 'only-on-failure',
    },
    webServer: {
        command: `node ../../runtime/wasm/serve.mjs . ${PORT}`,
        url: `http://127.0.0.1:${PORT}/index.html`,
        reuseExistingServer: false,
        timeout: 15_000,
    },
});
