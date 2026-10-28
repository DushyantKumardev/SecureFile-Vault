import js from '@eslint/js'
import globals from 'globals'
import reactHooks from 'eslint-plugin-react-hooks'
import reactRefresh from 'eslint-plugin-react-refresh'
import tseslint from 'typescript-eslint'
import { defineConfig, globalIgnores } from 'eslint/config'

export default defineConfig([
  globalIgnores(['dist']),
  {
    files: ['**/*.{ts,tsx}'],
    extends: [
      js.configs.recommended,
      tseslint.configs.recommended,
      reactHooks.configs.flat.recommended,
      reactRefresh.configs.vite,
    ],
    languageOptions: {
      globals: globals.browser,
    },
    rules: {
      'react-refresh/only-export-components': [
        'warn',
        { allowConstantExport: true },
      ],
    },
  },
  {
    // Disable Fast Refresh export restrictions for UI component primitives and context providers
    // where exporting variant helpers (e.g. CVA variants) or hooks alongside components is intentional.
    files: [
      'src/components/ui/**/*.{ts,tsx}',
      '**/components/ui/**/*.{ts,tsx}',
      'src/components/theme-provider.tsx',
      '**/components/theme-provider.tsx',
    ],
    rules: {
      'react-refresh/only-export-components': 'off',
    },
  },
])
