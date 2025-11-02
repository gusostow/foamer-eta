// API endpoint - defaults to localhost for local dev, replaced during deployment
const API_BASE_URL = 'http://localhost:8080';

// Get form elements
const form = document.getElementById('messageForm');
const passwordInput = document.getElementById('apiPassword');
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

    const password = passwordInput.value.trim();
    const content = messageInput.value.trim();

    // Client-side validation
    if (!password) {
        showStatus('Password is required', 'error');
        return;
    }

    if (!content || content.length > 96) {
        showStatus('Message must be between 1 and 96 characters', 'error');
        return;
    }

    // Disable form during submission
    submitBtn.disabled = true;
    passwordInput.disabled = true;
    messageInput.disabled = true;
    showStatus('Submitting...', 'info');

    try {
        const response = await fetch(`${API_BASE_URL}/messages`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'x-api-key': password,
            },
            body: JSON.stringify({ content }),
        });

        if (response.ok) {
            const data = await response.json();
            showStatus('Message submitted successfully!', 'success');

            // Clear message field (keep password for convenience)
            messageInput.value = '';
            charCount.textContent = '0';
        } else if (response.status === 403) {
            showStatus('Authentication failed: Invalid password', 'error');
        } else {
            const errorText = await response.text();
            showStatus(`Error: ${errorText}`, 'error');
        }
    } catch (error) {
        showStatus(`Network error: ${error.message}`, 'error');
    } finally {
        // Re-enable form
        submitBtn.disabled = false;
        passwordInput.disabled = false;
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
