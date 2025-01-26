use dialoguer::{Select, Input, Password};
use sss_rs::prelude::*;
// use std::io::Cursor;
use aes_gcm::{Aes256Gcm, aead::Aead, KeyInit};
use aes::cipher::generic_array::GenericArray;
use pbkdf2::pbkdf2_hmac;
use sha2::{Sha256, Digest};
use serde::{Serialize, Deserialize};
use std::iter;
use std::time::{SystemTime, UNIX_EPOCH};

#[derive(Serialize, Deserialize, Debug)]
enum SecretType {
    String(String),
    Int(i64),
    Float(f64),
}

const SALT_LEN: usize = 16;
const NONCE_LEN: usize = 12;
const MIN_SECRET_SIZE: usize = 32;

fn main() {
    let mode = Select::new()
        .with_prompt("Choose operation")
        .items(&["Encrypt", "Decrypt"])
        .interact()
        .unwrap();

    match mode {
        0 => encrypt_flow(),
        1 => decrypt_flow(),
        _ => unreachable!(),
    }
}

fn encrypt_flow() {
    let secret = Input::<String>::new()
        .with_prompt("Enter secret value")
        .interact()
        .unwrap();

    let secret_data = match secret.parse::<i64>() {
        Ok(i) => SecretType::Int(i),
        Err(_) => match secret.parse::<f64>() {
            Ok(f) => SecretType::Float(f),
            Err(_) => SecretType::String(secret),
        },
    };

    let mut bytes = serialize_secret(secret_data);
    pad_to_minimum(&mut bytes);
    
    let password = Password::new()
        .with_prompt("Enter encryption password")
        .interact()
        .unwrap();

    let (encrypted_data, salt, nonce) = encrypt_data(&bytes, &password);

    // Prepend salt and nonce to encrypted data
    let mut combined_data = Vec::new();
    combined_data.extend_from_slice(&salt);
    combined_data.extend_from_slice(&nonce);
    combined_data.extend_from_slice(&encrypted_data);

    let total_shares: u8 = Input::new()
        .with_prompt("Total number of shares")
        .interact()
        .unwrap();

    let threshold: u8 = Input::new()
        .with_prompt("Minimum shares required")
        .interact()
        .unwrap();

    let shares = share(&combined_data, threshold, total_shares, false)
        .expect("Failed to create shares");

    println!("\nSAVE THESE VALUES FOR DECRYPTION:");
    println!("Salt: {}", hex::encode(salt));
    println!("Nonce: {}", hex::encode(nonce));
    println!("\nGenerated shares:");
    for share in &shares {
        let mut hasher = Sha256::new();
        hasher.update(share.as_slice());
        let hash_bytes = hasher.finalize();
        let short_hash_hex = hex::encode(&hash_bytes[..4]);

        println!("Share ID '{:?}': {:?}", short_hash_hex, share);
    }
}

fn decrypt_flow() {
    let shares: Vec<String> = Input::<String>::new()
        .with_prompt("Enter shares (comma separated)")
        .interact_text()
        .unwrap()
        .split(',')
        .map(|s| s.trim().to_string())
        .collect();

    let password = Password::new()
        .with_prompt("Enter encryption password")
        .interact()
        .unwrap();

    let encrypted_data = reconstruct(&shares, false)
        .expect("Failed to recover secret");

    // Extract salt and nonce from the beginning of the encrypted data
    let salt = &encrypted_data[..SALT_LEN];
    let nonce = &encrypted_data[SALT_LEN..SALT_LEN + NONCE_LEN];
    let encrypted_data = &encrypted_data[SALT_LEN + NONCE_LEN..];

    let decrypted_data = decrypt_data(
        encrypted_data,
        &password,
        salt,
        nonce
    ).expect("Decryption failed");

    let secret = deserialize_secret(&decrypted_data);
    
    println!("\nRecovered secret:");
    match secret {
        SecretType::String(s) => println!("{}", s),
        SecretType::Int(i) => println!("{}", i),
        SecretType::Float(f) => println!("{}", f),
    }
}

fn encrypt_data(data: &[u8], password: &str) -> (Vec<u8>, [u8; SALT_LEN], [u8; NONCE_LEN]) {
    let timestamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .expect("Time went backwards")
        .as_millis();

    let mut hasher = Sha256::new();
    hasher.update(timestamp.to_be_bytes());
    let hash_bytes = hasher.finalize();

    let mut salt = [0u8; SALT_LEN];
    salt.copy_from_slice(&hash_bytes[..SALT_LEN]);

    let mut nonce = [0u8; NONCE_LEN];
    nonce.copy_from_slice(&hash_bytes[SALT_LEN..SALT_LEN + NONCE_LEN]);

    let mut key = [0u8; 32];
    pbkdf2_hmac::<Sha256>(password.as_bytes(), &salt, 100_000, &mut key);

    let key_array = GenericArray::from_slice(&key);

    let encrypted_data = Aes256Gcm::new(key_array)
        .encrypt(GenericArray::from_slice(&nonce), data)
        .expect("Encryption failed");

    (encrypted_data, salt, nonce)
}

fn decrypt_data(data: &[u8], password: &str, salt: &[u8], nonce: &[u8]) -> Result<Vec<u8>, aes_gcm::Error> {
    let mut key = [0u8; 32];
    pbkdf2_hmac::<Sha256>(password.as_bytes(), salt, 100_000, &mut key);

    let key_array = GenericArray::from_slice(&key);
    Aes256Gcm::new(key_array)
        .decrypt(GenericArray::from_slice(nonce), data)
}

fn serialize_secret(secret: SecretType) -> Vec<u8> {
    let mut bytes = vec![];
    match secret {
        SecretType::String(s) => {
            bytes.push(0u8);
            bytes.extend_from_slice(&(s.len() as u32).to_be_bytes());
            bytes.extend_from_slice(s.as_bytes());
        }
        SecretType::Int(i) => {
            bytes.push(1u8);
            bytes.extend_from_slice(&i.to_be_bytes());
        }
        SecretType::Float(f) => {
            bytes.push(2u8);
            bytes.extend_from_slice(&f.to_be_bytes());
        }
    }
    bytes
}

fn deserialize_secret(bytes: &[u8]) -> SecretType {
    let type_byte = bytes[0];
    match type_byte {
        0 => {
            let len = u32::from_be_bytes(bytes[1..5].try_into().unwrap()) as usize;
            let s = String::from_utf8(bytes[5..5+len].to_vec()).unwrap();
            SecretType::String(s)
        }
        1 => {
            let i = i64::from_be_bytes(bytes[1..9].try_into().unwrap());
            SecretType::Int(i)
        }
        2 => {
            let f = f64::from_be_bytes(bytes[1..9].try_into().unwrap());
            SecretType::Float(f)
        }
        _ => panic!("Invalid type byte"),
    }
}

fn pad_to_minimum(bytes: &mut Vec<u8>) {
    if bytes.len() < MIN_SECRET_SIZE {
        let padding = MIN_SECRET_SIZE - bytes.len();
        bytes.extend(iter::repeat(0u8).take(padding));
    }
}
