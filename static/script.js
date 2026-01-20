/**
 * ShareMap Configuration Interface - Client-side JavaScript
 */

// Toggle group collapse
function toggleGroup(element) {
    element.classList.toggle('collapsed');
    const content = element.closest('.card').querySelector('.group-content');
    content.classList.toggle('collapsed');
}

// Handle mutex (mutual exclusion) constraints
function handleMutex(checkbox) {
    const mutexField = checkbox.dataset.mutex;
    if (!mutexField) return;
    
    const mutexCheckbox = document.getElementById(mutexField);
    if (!mutexCheckbox) return;
    
    // If this checkbox is being enabled, disable the mutex partner
    if (checkbox.checked) {
        mutexCheckbox.checked = false;
        updateToggleLabel(mutexCheckbox);
    }
    
    updateToggleLabel(checkbox);
}

// Update toggle label text
function updateToggleLabel(checkbox) {
    const label = checkbox.parentElement.querySelector('.toggle-label');
    if (label) {
        label.textContent = checkbox.checked ? 'Enabled' : 'Disabled';
    }
}

// Validate numeric input
function validateNumeric(input) {
    const value = parseFloat(input.value);
    const min = input.dataset.min !== undefined ? parseFloat(input.dataset.min) : null;
    const max = input.dataset.max !== undefined ? parseFloat(input.dataset.max) : null;
    const errorEl = document.getElementById('error-' + input.id);
    
    let isValid = true;
    let errorMsg = '';
    
    if (isNaN(value)) {
        isValid = false;
        errorMsg = 'Please enter a valid number';
    } else if (min !== null && value < min) {
        isValid = false;
        errorMsg = `Value must be at least ${min}`;
    } else if (max !== null && value > max) {
        isValid = false;
        errorMsg = `Value must be at most ${max}`;
    }
    
    input.classList.toggle('error', !isValid);
    input.classList.toggle('valid', isValid);
    
    if (errorEl) {
        errorEl.textContent = errorMsg;
        errorEl.classList.toggle('visible', !isValid);
    }
    
    return isValid;
}

// Validate all fields
async function validateAll() {
    const form = document.getElementById('configForm');
    const formData = new FormData(form);
    const config = {};
    
    // Build config object from form
    for (const [key, value] of formData.entries()) {
        if (key === 'udp_host' || key === 'udp_port') continue;
        
        // Handle booleans
        const input = document.getElementById(key);
        if (input && input.type === 'checkbox') {
            config[key] = input.checked;
        } else {
            // Try to parse as number
            const numVal = parseFloat(value);
            config[key] = isNaN(numVal) ? value : numVal;
        }
    }
    
    // Handle unchecked checkboxes
    document.querySelectorAll('input[type="checkbox"]').forEach(cb => {
        if (cb.name !== 'udp_host' && cb.name !== 'udp_port') {
            if (!config.hasOwnProperty(cb.name)) {
                config[cb.name] = false;
            }
        }
    });
    
    try {
        const response = await fetch('/api/validate', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(config)
        });
        
        const result = await response.json();
        displayValidationResults(result);
    } catch (error) {
        console.error('Validation error:', error);
        displayValidationResults({
            valid: false,
            errors: { '_general': ['Failed to validate: ' + error.message] }
        });
    }
}

// Display validation results
function displayValidationResults(result) {
    const summary = document.getElementById('validation-summary');
    const content = document.getElementById('validation-content');
    
    summary.style.display = 'block';
    summary.classList.remove('success', 'error');
    summary.classList.add(result.valid ? 'success' : 'error');
    
    // Clear previous field highlighting
    document.querySelectorAll('.form-group input, .form-group select').forEach(el => {
        el.classList.remove('error', 'valid');
    });
    
    if (result.valid) {
        content.innerHTML = '<p style="color: #166534;">✓ All fields are valid! Configuration is ready to send.</p>';
    } else {
        let html = '<ul>';
        for (const [field, errors] of Object.entries(result.errors)) {
            if (field === '_mutex') {
                errors.forEach(err => {
                    html += `<li style="color: #991b1b;">⚠ ${err}</li>`;
                });
            } else if (field === '_general') {
                errors.forEach(err => {
                    html += `<li style="color: #991b1b;">⚠ ${err}</li>`;
                });
            } else {
                const input = document.getElementById(field);
                if (input) {
                    input.classList.add('error');
                }
                errors.forEach(err => {
                    html += `<li style="color: #991b1b;"><strong>${field}:</strong> ${err}</li>`;
                });
            }
        }
        html += '</ul>';
        content.innerHTML = html;
    }
    
    // Scroll to summary
    summary.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
}

// Reset form to defaults
async function resetToDefaults() {
    if (!confirm('Reset all fields to their default values?')) {
        return;
    }
    
    try {
        const response = await fetch('/api/defaults');
        const defaults = await response.json();
        
        for (const [field, value] of Object.entries(defaults)) {
            const input = document.getElementById(field);
            if (!input) continue;
            
            if (input.type === 'checkbox') {
                input.checked = Boolean(value);
                updateToggleLabel(input);
            } else {
                input.value = value;
            }
            
            // Clear validation state
            input.classList.remove('error', 'valid');
            const errorEl = document.getElementById('error-' + field);
            if (errorEl) {
                errorEl.classList.remove('visible');
            }
        }
        
        // Hide validation summary
        document.getElementById('validation-summary').style.display = 'none';
        
        // Show success message
        showFlash('success', 'Form reset to default values');
    } catch (error) {
        console.error('Reset error:', error);
        showFlash('error', 'Failed to reset form: ' + error.message);
    }
}

// Show a flash message
function showFlash(type, message) {
    const container = document.querySelector('.flash-messages') || createFlashContainer();
    
    const flash = document.createElement('div');
    flash.className = `flash ${type}`;
    flash.innerHTML = `
        <span class="flash-icon">${type === 'success' ? '✓' : '⚠'}</span>
        ${message}
        <button class="flash-close" onclick="this.parentElement.remove()">×</button>
    `;
    
    container.appendChild(flash);
    
    // Auto-remove after 5 seconds
    setTimeout(() => {
        if (flash.parentElement) {
            flash.remove();
        }
    }, 5000);
}

function createFlashContainer() {
    const container = document.createElement('div');
    container.className = 'flash-messages';
    document.querySelector('.container').insertBefore(container, document.querySelector('form'));
    return container;
}

// Scientific notation formatter for display
function formatScientific(value) {
    const num = parseFloat(value);
    if (isNaN(num)) return value;
    
    if (Math.abs(num) >= 1e6 || (Math.abs(num) < 1e-3 && num !== 0)) {
        return num.toExponential(2);
    }
    return num.toString();
}

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    // Add change listeners to all toggle checkboxes
    document.querySelectorAll('.toggle input[type="checkbox"]').forEach(checkbox => {
        checkbox.addEventListener('change', function() {
            updateToggleLabel(this);
        });
    });
    
    // Add input validation to numeric fields
    document.querySelectorAll('.numeric-input').forEach(input => {
        input.addEventListener('blur', function() {
            validateNumeric(this);
        });
    });
    
    // Form submission validation
    document.getElementById('configForm').addEventListener('submit', function(e) {
        let hasErrors = false;
        
        // Validate all numeric inputs
        document.querySelectorAll('.numeric-input').forEach(input => {
            if (!validateNumeric(input)) {
                hasErrors = true;
            }
        });
        
        if (hasErrors) {
            e.preventDefault();
            showFlash('error', 'Please fix validation errors before submitting');
        }
    });
});
