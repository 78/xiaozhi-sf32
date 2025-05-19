import { navbar } from "vuepress-theme-hope";

export const zhNavbar = navbar([
  "/",
  {
    text: "快速入门",
    icon: "lightbulb",
    prefix: "/get-started/",
    children: [
      "SF32LB52-DevKit-ULP/README.md",
      "SF32LB52-DevKit-LCD/README.md",
      "SF32LB52-DevKit-Nano/README.md",
    ],
  },
  {
    text: "源码构建",
    icon: "code",
    link: "/source-build/",
  }
]);
