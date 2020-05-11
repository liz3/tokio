const { execFile } = require("child_process");
const fs = require("fs");
const path = require("path");
let loaded: boolean = false;

export function prepareFile(
  url: string,
  dir: string = path.join(__dirname, ".dis-light-tmpdownloads")
): Promise<string> {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(dir)) {
      fs.mkdirSync(dir, { recursive: true });
    }
    const id = `dl-${Date.now()}`;
    const execPath = path.join(__dirname, "..", "..", "..", "youtube-dl");
    if (!fs.existsSync(execPath)) {
      reject(new Error("yotuobe-dl executable not found"));
      return;
    }
    const process = execFile(
      execPath,
      [
        "-x",
        "--audio-format",
        "opus",
        "-o",
        path.join(dir, `${id}.%(ext)s`),
        url,
      ],
      (error, stdout, stderr) => {
        if (error) {
          reject(error);
          return;
        }
        resolve(path.join(dir, `${id}.opus`));
      }
    );
  });
}
