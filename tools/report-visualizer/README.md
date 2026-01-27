# CCExtractor Report Visualizer

A web-based tool to visualize CCExtractor's `-out=report` output.

## Features

- Upload and parse report.txt files
- Visual display of subtitle pages
- Timeline visualization with Chart.js
- Shows page types, timestamps, and activity

## Usage

1. Run CCExtractor with `-out=report` option:
```bash
   ccextractor input.ts -out=report
```

2. Open `index.html` in your browser
3. Upload the generated report.txt file
4. View the visualized results

## Demo

You can host this on GitHub Pages or run it locally by simply opening `index.html` in any modern web browser.

## Technologies

- HTML5
- CSS3
- JavaScript (Vanilla)
- Chart.js for visualizations

## Testing

The tool works entirely client-side and requires no server setup. Simply open the HTML file and upload a report file to test.
