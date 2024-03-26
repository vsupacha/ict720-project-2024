import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision
import cv2
from sklearn import tree

# init webcam
cam = cv2.VideoCapture(0)
# init mediapipe
base_options = python.BaseOptions(model_asset_path='./hand_landmarker.task')
options = vision.HandLandmarkerOptions(base_options=base_options, num_hands=1)
detector = vision.HandLandmarker.create_from_options(options)
# init mode
ai_mode = 'none'
ai_features = {'rock': [], 'paper': [], 'scissors': []}
ai_model = None
while True:
    ret, frame = cam.read()
    if not ret:
        break
    frame = cv2.flip(frame, 1)
    cv2.imshow("frame", frame)
    key_pressed = cv2.waitKey(1) & 0xFF
    # press q to quit 
    if key_pressed == ord("q"):
        break
    # press r to label rock
    elif key_pressed == ord("r"):
        ai_mode = 'rock'
        print("Label mode: ", ai_mode)
    # press p to label paper
    elif key_pressed == ord("p"):
        ai_mode = 'paper'
        print("Label mode: ", ai_mode)
    # press s to label scissors
    elif key_pressed == ord("s"):
        ai_mode = 'scissors'
        print("Label mode: ", ai_mode)
    # press t to train
    elif key_pressed == ord("t"):
        print("Training...")
        X = []
        Y = []
        for label in ai_features.keys():
            for feature in ai_features[label]:
                X.append(feature)
                Y.append(label)
        ai_model = tree.DecisionTreeClassifier()
        ai_model = ai_model.fit(X, Y)
        print("Training done.")
    # press d to detect
    elif key_pressed == ord("d"):
        if ai_model is not None:
            img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
            img = mp.Image(image_format=mp.ImageFormat.SRGB, data=img)
            result = detector.detect(img)
            if len(result.handedness) > 0:
                feature = [landmark.x for landmark in result.hand_landmarks[0]] + [landmark.y for landmark in result.hand_landmarks[0]]
                prediction = ai_model.predict([feature])
                print(prediction)
        else:
            print("Model not trained yet.")    
    elif key_pressed == ord(" ") and ai_mode != 'none':
        img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img = mp.Image(image_format=mp.ImageFormat.SRGB, data=img)
        result = detector.detect(img)
        if len(result.handedness) > 0:
            feature = [landmark.x for landmark in result.hand_landmarks[0]] + [landmark.y for landmark in result.hand_landmarks[0]]
            ai_features[ai_mode].append(feature)
            print(f"Feature added to {ai_mode}: {len(ai_features[ai_mode])}")
    else:
        pass
cam.release()