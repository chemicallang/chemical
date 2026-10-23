// Trivially copyable marker — structs that can be memcpy'd without
// running a custom copy-constructor.
public interface Copy {}

// Auto traits: a type is Send when its values may move to another thread, and
// Sync when shared references to it may be used from another thread. These are
// derived structurally from fields (raw pointers are neither).
public interface Send {}
public interface Sync {}