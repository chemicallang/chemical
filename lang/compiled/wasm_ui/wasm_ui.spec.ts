import { test, expect } from '@playwright/test';

test('wasm counter demo increments in Chromium', async ({ page }) => {
    const problems: string[] = [];
    page.on('pageerror', (error) => problems.push(`pageerror: ${error.message}`));
    page.on('console', (message) => {
        if (message.type() === 'error') problems.push(`console: ${message.text()}`);
    });

    await page.goto('/index.html');

    const counter = page.locator('.wasm-counter span');
    const button = page.locator('.wasm-counter button');
    await expect(counter).toHaveText('0');
    await expect(button).toHaveText('Increment');

    await button.click();
    await expect(counter).toHaveText('1');
    await button.click();
    await button.click();
    await expect(counter).toHaveText('3');

    // The module must not need a trap fallback or print a panic.
    await expect(page.locator('body')).not.toContainText('panic:');
    expect(problems).toEqual([]);
});
