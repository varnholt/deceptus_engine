import numpy as np
from PIL import Image
import matplotlib.pyplot as plt

def normalize_vectors(U, V):
    magnitude = np.sqrt(U**2 + V**2) + 1e-8
    return U / magnitude, V / magnitude

def encode_vector_field(U, V):
    U_norm, V_norm = normalize_vectors(U, V)
    R = ((U_norm + 1) / 2 * 255).astype(np.uint8)
    G = ((V_norm + 1) / 2 * 255).astype(np.uint8)
    B = np.zeros_like(R, dtype=np.uint8)
    return np.stack([R, G, B], axis=-1)

def generate_raw_fields(size):
    x = np.linspace(-1, 1, size)
    y = np.linspace(-1, 1, size)
    X, Y = np.meshgrid(x, y)

    # Gradient: toward center
    U_grad = -X
    V_grad = -Y

    # Swirl: clockwise rotation
    U_swirl = Y
    V_swirl = -X

    # Mixed: spiral inward
    U_mixed = U_grad + U_swirl
    V_mixed = V_grad + V_swirl

    return (X, Y), (U_grad, V_grad), (U_swirl, V_swirl), (U_mixed, V_mixed)

def save_encoded_image(U, V, filename):
    encoded = encode_vector_field(U, V)
    img = Image.fromarray(encoded, mode='RGB')
    img.save(filename)

def save_arrow_plot(X, Y, U, V, filename, title, step=8):
    # Subsample grid
    X_sub = X[::step, ::step]
    Y_sub = Y[::step, ::step]
    U_sub, V_sub = normalize_vectors(U[::step, ::step], V[::step, ::step])

    plt.figure(figsize=(6, 6))
    plt.quiver(X_sub, Y_sub, U_sub, V_sub, color='black', scale=20)
    plt.title(title)
    plt.axis('equal')
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(filename, dpi=300)
    plt.close()

def main():
    size = 256
    (X, Y), (U_grad, V_grad), (U_swirl, V_swirl), (U_mixed, V_mixed) = generate_raw_fields(size)

    # Save encoded PNGs
    save_encoded_image(U_grad, V_grad, 'texture1_gradient.png')
    save_encoded_image(U_swirl, V_swirl, 'texture2_swirl.png')
    save_encoded_image(U_mixed, V_mixed, 'texture3_mixed.png')

    # Save arrow previews
    save_arrow_plot(X, Y, U_grad, V_grad, 'texture1_gradient_arrows.png', 'Gradient to Center')
    save_arrow_plot(X, Y, U_swirl, V_swirl, 'texture2_swirl_arrows.png', 'Clockwise Swirl')
    save_arrow_plot(X, Y, U_mixed, V_mixed, 'texture3_mixed_arrows.png', 'Mixed Spiral')

if __name__ == "__main__":
    main()
