import { CIPHER_CONFIG } from "@/constants";

declare global {
  interface Window {
    chrome?: {
      webview?: {
        postMessage: (message: string) => void;
      };
    };
    setFile?: (filePath: string) => void;
    setStatus?: (message: string, success: boolean | null) => void;
  }
}

/**
 * Send action payload to the native C++ backend via Microsoft WebView2 bridge.
 * If running in standalone browser dev mode (e.g. Vite dev server), logs gracefully.
 */
export function sendToBackend(action: string, payload: Record<string, unknown> = {}): void {
  const message = JSON.stringify({ action, ...payload });
  if (window.chrome?.webview?.postMessage) {
    window.chrome.webview.postMessage(message);
  } else {
    console.info(`[WebView2 Dev Fallback] Outgoing action: ${action}`, payload);
  }
}

export function parseFileMetadata(fullPath: string) {
  if (!fullPath) return null;
  const normalized = fullPath.replace(/\\/g, '/');
  const fileName = normalized.split('/').pop() || fullPath;
  const match = fileName.match(/\.([^.]+)$/);
  const extension = match ? match[1].toUpperCase() : 'FILE';
  const isEncrypted = extension === 'ENC' || fileName.toLowerCase().endsWith('.enc');
  const directory = normalized.includes('/') ? normalized.substring(0, normalized.lastIndexOf('/')) : '';

  return {
    rawPath: fullPath,
    fileName,
    extension,
    isEncrypted,
    directory
  };
}

export function computePasswordEntropy(pwd: string) {
  if (!pwd) {
    return {
      score: 0,
      label: 'None' as const,
      percent: 0,
      checks: { length: false, number: false, upper: false, symbol: false }
    };
  }

  const checks = {
    length: pwd.length >= CIPHER_CONFIG.MIN_KEY_LENGTH,
    number: /[0-9]/.test(pwd),
    upper: /[A-Z]/.test(pwd),
    symbol: /[^A-Za-z0-9]/.test(pwd)
  };

  let count = 0;
  if (checks.length) count++;
  if (checks.number) count++;
  if (checks.upper) count++;
  if (checks.symbol) count++;
  if (pwd.length >= CIPHER_CONFIG.STRONG_KEY_LENGTH) count++;

  let label: 'Weak' | 'Fair' | 'Strong' | 'Very Strong' = 'Weak';
  let percent = 25;

  if (count >= 4) {
    label = 'Very Strong';
    percent = 100;
  } else if (count === 3) {
    label = 'Strong';
    percent = 75;
  } else if (count === 2) {
    label = 'Fair';
    percent = 50;
  }

  return { score: count, label, percent, checks };
}

export function generateCryptographicKey(): string {
  const upper = 'ABCDEFGHJKLMNPQRSTUVWXYZ';
  const lower = 'abcdefghijkmnpqrstuvwxyz';
  const digits = '23456789';
  const symbols = '!@#$%^&*()_+';
  const all = upper + lower + digits + symbols;

  let key = '';
  key += upper[Math.floor(Math.random() * upper.length)];
  key += lower[Math.floor(Math.random() * lower.length)];
  key += digits[Math.floor(Math.random() * digits.length)];
  key += symbols[Math.floor(Math.random() * symbols.length)];

  for (let i = 4; i < CIPHER_CONFIG.GENERATED_KEY_LENGTH; i++) {
    key += all[Math.floor(Math.random() * all.length)];
  }

  return key.split('').sort(() => 0.5 - Math.random()).join('');
}
