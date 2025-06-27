// Import the functions you need from the SDKs you need
require("dotenv").config();
const firebase = require("firebase/app");
//const firebasedb = require('firebase/database')
const { getFirestore } = require("firebase/firestore");
// const firebaseConfig = {
//   apiKey: process.env.FIREBASE_API_KEY,
//   authDomain: process.env.FIREBASE_AUTH_DOMAIN,
//   databaseURL: process.env.FIREBASE_DATABASE_URL,
//   projectId: process.env.FIREBASE_PROJECT_ID,
//   storageBucket: process.env.FIREBASE_STORAGE_BUCKET,
//   messagingSenderId: process.env.FIREBASE_MESSAGING_SENDER_ID,
//   appId: process.env.FIREBASE_APP_ID,
//   measurementId: process.env.FIREBASE_MEASUREMENT_ID
// };
// Initialize Firebase


const firebaseConfig = {
  apiKey: "AIzaSyAh1iF_DmWmogEEdmk4UOG8Phdtgln134Y",
  authDomain: "smartlock-f37cb.firebaseapp.com",
  databaseURL: "https://smartlock-f37cb-default-rtdb.asia-southeast1.firebasedatabase.app",
  projectId: "smartlock-f37cb",
  storageBucket: "smartlock-f37cb.firebasestorage.app",
  messagingSenderId: "637740604909",
  appId: "1:637740604909:web:65d6943f8a2abdb63ddbe4",
  measurementId: "G-CRRCENZXT5"
};


const app = firebase.initializeApp(firebaseConfig);
const db = getFirestore(app);
const { 
  getAuth, 
  createUserWithEmailAndPassword, 
  signInWithEmailAndPassword, 
  signOut, 
  sendEmailVerification, 
  sendPasswordResetEmail,
  sendSignInLinkToEmail

} = require("firebase/auth") ;

module.exports = {
  getAuth,
  signInWithEmailAndPassword,
  createUserWithEmailAndPassword,
  signOut,
  sendEmailVerification,
  sendPasswordResetEmail,
  sendSignInLinkToEmail,
  db
};
