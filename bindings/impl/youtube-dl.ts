const { execFile } = require("child_process");
const fs = require("fs");
const path = require("path");
let loaded: boolean = false;

export function prepareFile(
  url: string,
  dir: string = path.join(__dirname, ".dis-light-tmpdownloads")
): Promise<any> {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
    const id = `dl-${Date.now()}`;
    let execPath = path.join(__dirname, "..", "..", "..", "youtube-dl");
    if (!fs.existsSync(execPath)) {
      execPath = "/opt/homebrew/bin/youtube-dl";
    }
    const process = execFile(
      execPath,
      ["--dump-json", url],
      (error, stdout, stderr) => {
        resolve(JSON.parse(stdout));
      }
    );
  });
}
