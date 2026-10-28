import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { ThemeProvider } from '@/components/theme-provider'
import { APP_CONFIG } from '@/constants'
import './index.css'
import App from './App.tsx'

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <ThemeProvider defaultTheme="system" storageKey={APP_CONFIG.storage.THEME}>
      <App />
    </ThemeProvider>
  </StrictMode>,
)
