// ============================================================================
// TLS Library Entry Point - Module Lifecycle
// ============================================================================
// ensure_init() is defined in ssl_ciphersuites.ch for proper dependency order.
// This file is auto-imported by the chemical.mod source directive.

public namespace tls {

    // Module-level initialization
    // tls_init() is a public API for explicit initialization
    public func tls_init() {
        ensure_init()
    }

} // namespace tls
