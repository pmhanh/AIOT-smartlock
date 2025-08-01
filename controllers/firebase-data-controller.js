const { 
  getAuth, 
  db
} = require('../config/firebase');

const { getFirestore, doc, setDoc, getDoc, updateDoc, collection, data, addDoc, arrayUnion } = require('firebase/firestore');
const { lock } = require('../routes');
const nodemailer = require('nodemailer')
const transporter = nodemailer.createTransport({
  service: 'gmail',
  auth: {
    user: 'underwavecontact@gmail.com',
    pass: 'awrj ukks lynl sslx'
  },
});


//const Pushsafer = require("pushsafer-notifications");

// Initialize Pushsafer with API Key
// const push = new Pushsafer({
//   k: process.env.PUSHSAFER_API_KEY,  // Use .env for security
// });

const getCurrentUserData = async (req, res) => {
  try {
      const userId = req.cookies.userId;
      if (!userId) return res.redirect('/login');
      const docRef = doc(db, `users/${userId}`);
      const userDoc = await getDoc(docRef);
      const userData = userDoc.data();
      return res.render('profile', {user: userData, userId: userId})
  } catch (error) {
      console.error("Error fetching user data:", error);
      return res.status(500).json({ message: "Error fetching user data" });
  }
};

const updateUsername = async (req, res) => {
  const userId = req.cookies.userId;
  if (!userId) return res.redirect('/login');
  const newUsername = req.body.username;
  try {
      const docRef = doc(db, `users/${userId}`);
      await updateDoc(docRef, { username: newUsername });
      res.redirect(`/profile`);
  } catch (error) {
      console.error("Error updating username:", error);
      return res.status(500).json({ message: "Error updating username" });
  }
}


// const addHistoryEntry = async (req, res) => {
//   try {
//     const user = getAuth().currentUser;

//     if (!user) {
//       return res.status(401).json({ message: "Unauthorized" });
//     }

//     const user_email = user.email;
//     const userId = user.uid;

//     const docRef = doc(db, `history/${userId}`);

//     // Adding a new history entry to the array
//     const newHistoryEntry = {
//       email: user_email,
//       date_time: req.body.date_time,
//       state: req.body.state,
//       image: req.body.image,
//       result: req.body.result,
//     };

//     const snapshot = await getDoc(docRef)
      
//     if (!snapshot.exists()) {
//       // Create Firestore document after email verification
//       await setDoc(docRef, {
//       });
//     }
//     // Use updateDoc and arrayUnion to append to the array
//     await updateDoc(docRef, {
//       entries: arrayUnion(newHistoryEntry), // 'entries' is the array field in the document
//     });

//     res.status(200).json({
//       message: "History entry added successfully!",
//     });
//   } catch (error) {
//     console.error("Error adding history entry:", error);
//     res.status(500).json({
//       message: "Failed to add history entry.",
//       error: error.message,
//     });
//   }
// };


const sendEmail = async(state) => {
  user = getAuth().currentUser;
  const changepass = {
    from: 'underwavecontact@gmail.com',
    to: user.email,
    subject: 'Password changed successfully',
    text: 'Your lock password has been successfully changed.',
    };
  if (state == "Change password")
  {
    await transporter.sendMail(changepass);
  }
  const addfinger = {
    from: 'underwavecontact@gmail.com',
    to: user.email,
    subject: 'Finger added successfully',
    text: 'Your finger has been successfully added to your account.',
  }    
  if (state == "Add fingerprint")
  {
    await transporter.sendMail(addfinger);
  }
}

const addHistoryEntry = async(data) => {
  try {
    const user = getAuth().currentUser;

    if (!user) {
      console.error("Unauthorized: No current user.");
      return;    
    }

    const user_email = user.email;
    const userId = user.uid;

    const docRef = doc(db, `history/${userId}`);

    // Adding a new history entry to the array
    let newHistoryEntry = data;
    newHistoryEntry.email = user_email;
    console.log(newHistoryEntry)
    sendEmail(data.state)

    const snapshot = await getDoc(docRef)
      
    if (!snapshot.exists()) {
      // Create Firestore document after email verification
      await setDoc(docRef, {
      });
    }
    // Use updateDoc and arrayUnion to append to the array
    await updateDoc(docRef, {
      entries: arrayUnion(newHistoryEntry), // 'entries' is the array field in the document
    });

    
  } catch (error) {
    console.error("Error adding history entry:", error);
    
  }
};
const getHistoryEntry = async(req, res) =>{
  try {
    const userId = req.cookies.userId;
    if (!userId) return res.redirect('/login');
    const docRef = doc(db, `history/${userId}`);
    const userInfo = (await getDoc(doc(db, `users/${userId}`))).data()
    const userDoc = await getDoc(docRef);
    let userHistory = [];
    if (userDoc.data() && userDoc.data().entries){
      userHistory = userDoc.data().entries;
      // Sắp xếp theo thời gian mới nhất trước
      userHistory.sort((a, b) => {
        const dateA = new Date(a.date_time);
        const dateB = new Date(b.date_time);
        return dateB - dateA; // Sắp xếp giảm dần (mới nhất trước)
      });
    }
    return res.render('history', { history: userHistory , user: userInfo, userId : userId});
  } catch (error) {
    console.error('Error fetching user history:', error);
    res.status(500).send('Error fetching user history.');
  }
}


const getMonitor = async(req, res) => {
  try {
    const userId = req.cookies.userId;
    if (!userId) return res.redirect('/login');
    const docRef = doc(db, `users/${userId}`);
    const hisRef = doc(db, `history/${userId}`);
    const userDoc = await getDoc(docRef);
    const hisDoc = await getDoc(hisRef)
    const userData = userDoc.data();
    return res.render('monitor', {user: userData, userId: userId})
  } catch (error) {
      console.error("Error fetching user data:", error);
      return res.status(500).json({ message: "Error fetching user data" });
  }
}


const sendNotificationtoPhone = async(req, res) => {
  const { message, title } = req.body;

  const notificationData = {
    m: message || "Default notification message!",
    t: title || "Web App Alert",
    d: "",  // Leave empty to send to all devices
    i: "1",  // Icon ID
    s: "5",  // Sound ID
    v: "3",  // Vibration strength
    u: "https://yourwebsite.com",  // Optional URL
    ut: "View Details",  // URL Title
  };

  const auth = getAuth()
  const userId = auth.currentUser.uid
  const docRef = doc(db, `users/${userId}`);

  const userDoc = await getDoc(docRef);
  console.log(userDoc)
  const userData = userDoc.data();
  console.log(userData)
  push.send(notificationData, (err, result) => {
    if (err) {
      console.error("Pushsafer Error:", err);
      return res.status(500).json({ error: "Failed to send notification" });
    }
    console.log("Pushsafer Response:", result);
    res.json({ success: true, message: "Notification sent successfully!" });
  });

}



module.exports = {
  getCurrentUserData, updateUsername, addHistoryEntry, getHistoryEntry, getMonitor, sendNotificationtoPhone
};