function loadFile() {
    const fileInput = document.getElementById('fileInput');
    const file = fileInput.files[0];
    
    if (!file) {
        alert('Please select a file first');
        return;
    }
    
    const reader = new FileReader();
    reader.onload = function(e) {
        const content = e.target.result;
        parseReport(content);
    };
    reader.readAsText(file);
}

function parseReport(content) {
    const lines = content.split('\n');
    const pages = {};
    let currentPage = null;
    
    // Parse the report file
    lines.forEach(line => {
        // Detect page numbers (e.g., "Page 150", "Teletext page 888")
        const pageMatch = line.match(/[Pp]age\s+(\d+)/);
        if (pageMatch) {
            currentPage = pageMatch[1];
            if (!pages[currentPage]) {
                pages[currentPage] = {
                    number: currentPage,
                    type: '',
                    timestamps: [],
                    details: []
                };
            }
        }
        
        // Extract type information
        const typeMatch = line.match(/[Tt]ype:\s*(.+)/);
        if (typeMatch && currentPage) {
            pages[currentPage].type = typeMatch[1].trim();
        }
        
        // Extract timestamps (HH:MM:SS format)
        const timestampMatch = line.match(/(\d{2}:\d{2}:\d{2})/g);
        if (timestampMatch && currentPage) {
            pages[currentPage].timestamps.push(...timestampMatch);
        }
        
        // Collect other details
        if (currentPage && line.trim() && !pageMatch && !typeMatch) {
            pages[currentPage].details.push(line.trim());
        }
    });
    
    displayResults(pages);
    displayTimeline(pages);
}

function displayResults(pages) {
    const resultsDiv = document.getElementById('results');
    resultsDiv.innerHTML = '';
    
    if (Object.keys(pages).length === 0) {
        resultsDiv.innerHTML = '<p style="color: white; text-align: center;">No subtitle pages found in the report.</p>';
        return;
    }
    
    Object.values(pages).forEach(page => {
        const card = document.createElement('div');
        card.className = 'page-card';
        
        card.innerHTML = `
            <h3>Page ${page.number}</h3>
            <div class="detail"><strong>Type:</strong> ${page.type || 'Unknown'}</div>
            <div class="detail"><strong>Occurrences:</strong> ${page.timestamps.length}</div>
            <div class="detail"><strong>First Appearance:</strong> ${page.timestamps[0] || 'N/A'}</div>
            <div class="detail"><strong>Last Appearance:</strong> ${page.timestamps[page.timestamps.length - 1] || 'N/A'}</div>
        `;
        
        resultsDiv.appendChild(card);
    });
}

function displayTimeline(pages) {
    const timelineDiv = document.getElementById('timeline');
    
    if (Object.keys(pages).length === 0) {
        timelineDiv.innerHTML = '';
        return;
    }
    
    timelineDiv.innerHTML = '<h2>Subtitle Activity Timeline</h2><canvas id="timelineChart"></canvas>';
    
    const labels = Object.keys(pages).map(p => `Page ${p}`);
    const data = Object.values(pages).map(p => p.timestamps.length);
    
    const ctx = document.getElementById('timelineChart').getContext('2d');
    new Chart(ctx, {
        type: 'bar',
        data: {
            labels: labels,
            datasets: [{
                label: 'Number of Subtitle Occurrences',
                data: data,
                backgroundColor: 'rgba(102, 126, 234, 0.7)',
                borderColor: 'rgba(102, 126, 234, 1)',
                borderWidth: 2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: true,
            scales: {
                y: {
                    beginAtZero: true,
                    ticks: {
                        stepSize: 1
                    }
                }
            }
        }
    });
}
