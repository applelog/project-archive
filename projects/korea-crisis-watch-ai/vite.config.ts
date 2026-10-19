import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  build: {
    // Increase the chunk size warning limit to 1000kB (1MB) to suppress the warning
    chunkSizeWarningLimit: 1000,
  },
});