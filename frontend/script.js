// API endpoint - change this for production
const API_BASE_URL = 'http://localhost:8080';

// Get form elements
const form = document.getElementById('messageForm');
const messageInput = document.getElementById('messageContent');
const charCount = document.getElementById('charCount');
const submitBtn = document.getElementById('submitBtn');
const statusMessage = document.getElementById('statusMessage');

// Update character counter as user types
messageInput.addEventListener('input', () => {
    const length = messageInput.value.length;
    charCount.textContent = length;

    // Disable submit if empty or over limit
    submitBtn.disabled = length === 0 || length > 96;
});

// Handle form submission
form.addEventListener('submit', async (e) => {
    e.preventDefault();

    const content = messageInput.value.trim();

    // Client-side validation
    if (!content || content.length > 96) {
        showStatus('Message must be between 1 and 96 characters', 'error');
        return;
    }

    // Disable form during submission
    submitBtn.disabled = true;
    messageInput.disabled = true;
    showStatus('Submitting...', 'info');

    try {
        const response = await fetch(`${API_BASE_URL}/messages`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ content }),
        });

        if (response.ok) {
            const data = await response.json();
            showStatus('Message submitted successfully!', 'success');

            // Clear form
            messageInput.value = '';
            charCount.textContent = '0';
        } else {
            const errorText = await response.text();
            showStatus(`Error: ${errorText}`, 'error');
        }
    } catch (error) {
        showStatus(`Network error: ${error.message}`, 'error');
    } finally {
        // Re-enable form
        submitBtn.disabled = false;
        messageInput.disabled = false;
    }
});

// Show status message
function showStatus(message, type) {
    statusMessage.textContent = message;
    statusMessage.className = `status-message ${type}`;
    statusMessage.style.display = 'block';

    // Auto-hide success messages after 3 seconds
    if (type === 'success') {
        setTimeout(() => {
            statusMessage.style.display = 'none';
        }, 3000);
    }
}
