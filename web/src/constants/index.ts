/**
 * Centralized Application Constants
 *
 * Provides a single source of truth for application metadata,
 * configuration, storage keys, navigation routes, and defaults across the client.
 */

export const APP_NAME = "SecureFile Vault";
export const APP_SHORT_NAME = "SecureFile";
export const APP_VERSION = "1.0.0";
export const APP_DESCRIPTION = "High-performance cryptographic stream vault desktop application";
export const PLATFORM_INFO = "WebView2 • Win32";

/** LocalStorage and persistence keys */
export const STORAGE_KEYS = {
  AUDIT_LOGS: "sfv_audit_logs",
  THEME: "sfv-ui-theme",
} as const;

/** Audit trail configuration */
export const AUDIT_CONFIG = {
  MAX_RECORDS: 50,
  STORAGE_KEY: STORAGE_KEYS.AUDIT_LOGS,
} as const;

/** Navigation tab definitions and localized titles */
export const NAVIGATION_TABS = {
  VAULT: {
    id: "vault" as const,
    name: "Vault Operations",
    headerTitle: "SecureFile Vault",
  },
  HISTORY: {
    id: "history" as const,
    name: "Audit Trail",
    headerTitle: "Audit History",
  },
} as const;

/** System status and fallback messages */
export const STATUS_MESSAGES = {
  READY: "Ready",
  SYSTEM_READY: "System ready. Select a file to initiate encryption or decryption.",
  ENCRYPTING_STREAM: "Encrypting Binary Stream...",
  DECRYPTING_STREAM: "Decrypting Binary Stream...",
} as const;

/** Cryptographic parameters and constraints */
export const CIPHER_CONFIG = {
  MIN_KEY_LENGTH: 8,
  STRONG_KEY_LENGTH: 14,
  GENERATED_KEY_LENGTH: 16,
  STREAM_CHUNK_BYTES: 8192,
} as const;

/** Unified Application Configuration object */
export const APP_CONFIG = {
  name: APP_NAME,
  shortName: APP_SHORT_NAME,
  version: APP_VERSION,
  description: APP_DESCRIPTION,
  platform: PLATFORM_INFO,
  storage: STORAGE_KEYS,
  audit: AUDIT_CONFIG,
  tabs: NAVIGATION_TABS,
  status: STATUS_MESSAGES,
  cipher: CIPHER_CONFIG,
} as const;
