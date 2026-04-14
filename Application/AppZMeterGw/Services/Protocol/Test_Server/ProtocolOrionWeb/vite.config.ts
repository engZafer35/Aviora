import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

export default defineConfig({
  plugins: [react()],
  server: {
    /** VirtualBox / LAN: http://192.168.56.1:5173 gibi host IP’lerden erişim */
    host: true,
    port: 5173,
    strictPort: true,
    proxy: {
      "/ws": {
        target: "ws://127.0.0.1:8788",
        ws: true,
      },
    },
  },
  preview: {
    host: true,
    port: 4173,
  },
});
