import { existsSync } from "node:fs";
import { join, dirname } from "node:path";
import { execSync } from "node:child_process";
import { fileURLToPath } from "node:url";

// Get the directory name of the current module
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Get the project root directory (one level up from __dirname)
const projectRoot = join(__dirname, "..");
const addonPath = join(projectRoot, "build/Release/gst_kit.node");
const nodeModulesPath = join(projectRoot, "node_modules");
const nodeAddonApiPath = join(nodeModulesPath, "node-addon-api");

if (!existsSync(addonPath)) {
  console.log("GStreamer Kit native addon not found, building...");

  // Check if dependencies are installed
  if (!existsSync(nodeModulesPath) || !existsSync(nodeAddonApiPath)) {
    console.log("Dependencies not found, installing...");
    try {
      execSync("npm install", { stdio: "inherit", cwd: projectRoot });
    } catch (error) {
      console.error("Failed to install dependencies:", error.message);
      process.exit(1);
    }
  }

  try {
    execSync("npm run build:native", { stdio: "inherit", cwd: projectRoot });
  } catch (error) {
    console.error("Failed to build native addon:", error.message);
    console.error("Make sure you have the required system dependencies:");
    console.error("- GStreamer development libraries");
    console.error("- CMake");
    console.error("- A C++ compiler");
    process.exit(1);
  }

  execSync("npm run build:ts", { stdio: "inherit", cwd: projectRoot });
}
