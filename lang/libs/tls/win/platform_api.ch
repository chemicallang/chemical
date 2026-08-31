public namespace tls {

    // BCryptGenRandom is declared in the osrand library (shared OS entropy source).
    // tls delegates random_fill to osrand::random_fill.

}
