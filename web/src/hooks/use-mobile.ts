import * as React from "react"

const MOBILE_BREAKPOINT = 768

/**
 * Subscribes to window viewport changes across the mobile breakpoint threshold.
 *
 * @param onStoreChange - React callback triggered when external state changes.
 * @returns Cleanup function to remove event listeners.
 */
function subscribe(onStoreChange: () => void) {
  const mql = window.matchMedia(`(max-width: ${MOBILE_BREAKPOINT - 1}px)`)
  mql.addEventListener("change", onStoreChange)
  return () => mql.removeEventListener("change", onStoreChange)
}

/**
 * Returns whether the current window width is less than the mobile breakpoint.
 */
function getSnapshot(): boolean {
  return window.innerWidth < MOBILE_BREAKPOINT
}

/**
 * Server-side fallback snapshot (defaults to false for desktop).
 */
function getServerSnapshot(): boolean {
  return false
}

/**
 * Hook to reactively determine if the current viewport is mobile.
 * Uses `useSyncExternalStore` to avoid cascading renders and state synchronization issues.
 */
export function useIsMobile(): boolean {
  return React.useSyncExternalStore(subscribe, getSnapshot, getServerSnapshot)
}
