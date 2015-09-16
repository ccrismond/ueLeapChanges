// Fill out your copyright notice in the Description page of Project Settings.

#include "SpaceTimeWarping.h"
#include "CameraWarp.h"
#include "LeapMotionControllerPlugin.h"
#include "LeapMotionDevice.h"
#include "Leap.h"
#include "LeapMotionTypes.h"
#include "Engine.h"

// Sets default values
ACameraWarp::ACameraWarp()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACameraWarp::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ACameraWarp::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

}

/// <summary>
/// Estimates the transform of this gameObject at the specified time
/// </summary>
/// <returns>
/// A transform with leapTime == time only if interpolation was possible
/// </returns>
TransformData ALeapCameraAlignment::TransformAtTime(long time) {
	if (history.Count < 1) {
		Debug.LogWarning("NO HISTORY!");
		return new TransformData(){
			leapTime = 0,
			position = Vector3.zero,
			rotation = Quaternion.identity
		};
	}
	if (history[0].leapTime >= time) {
		// Expect this for initial frames with high latency
		if (history[0].leapTime > time) Debug.LogWarning("NO INTERPOLATION: Using earliest time = " + history[0].leapTime + " > time = " + time);
		return history[0];
	}
	int t = 1;
	while (t < history.Count &&
		history[t].leapTime <= time) {
		t++;
	}
	if (!(t < history.Count)) {
		// Expect this for initial frames which will have a very low frame rate
		if (history[history.Count - 1].leapTime < time) Debug.LogWarning("NO INTERPOLATION: Using most recent time = " + history[history.Count - 1].leapTime + " < time = " + time);
		return history[history.Count - 1];
	}

	return TransformData.Lerp(history[t - 1], history[t], time);
}


/// <summary>
/// Rewinds position of target relative to most recent point in history
/// </summary>
/// <remarks>
/// This method applies the same time difference logic using for time alignment,
/// but ignores the tweening settings
/// </remarks>
/// <param name="isLeftCenterRight">
/// Applies a left camera alignment if < 0,
/// applies a right camera alignment if > 0, 
/// and applies no alignment if == 0.
/// </param>
void ALeapCameraAlignment::RelativeRewind(Transform target, int isLeftCenterRight = 0) {
	TransformData past = TransformAtTime(imageRetriever.ImageNow() - (long)(rewindAdjust*frameLatency.value));

	// Rewind position and rotation
	target.rotation = past.rotation;
	target.position = past.position + past.rotation * Vector3.forward * deviceInfo.focalPlaneOffset;

	if (isLeftCenterRight < 0) {
		// Apply the left camera alignment
		target.position += past.rotation * Vector3.left * deviceInfo.baseline * 0.5f;
	}
	if (isLeftCenterRight > 0) {
		// Apply the right camera alignment
		target.position += past.rotation * Vector3.right * deviceInfo.baseline * 0.5f;
	}
}

void ALeapCameraAlignment::Start() {
	if (handController == null ||
		imageRetriever == null ||
		leftCamera == null ||
		rightCamera == null ||
		centerCamera == null) {
		Debug.LogWarning("HandController, ImageRetriever and Alignment Target references cannot be null -> enabled = false");
		enabled = false;
		return;
	}

	deviceInfo = (overrideDeviceType) ? new LeapDeviceInfo(overrideDeviceTypeWith) : handController.GetDeviceInfo();
	if (deviceInfo.type == LeapDeviceType.Invalid) {
		Debug.LogWarning("Invalid Leap Device");
		enabled = false;
		return;
	}

	history = new List<TransformData>();
	imageLatency = new SmoothedFloat(){
		delay = latencySmoothing
	};
	frameLatency = new SmoothedFloat(){
		delay = latencySmoothing
	};
}

//// IMPORTANT: This method MUST be called after OVRManager.LateUpdate.
//// Use EnableUpdateOrdering script to ensure correct call order.
void ALeapCameraAlignment::LateUpdate() {
	if (!(IsFinite(leftCamera.position) && IsFinite(leftCamera.rotation) &&
		IsFinite(centerCamera.transform.position) && IsFinite(centerCamera.transform.rotation) &&
		IsFinite(rightCamera.position) && IsFinite(rightCamera.rotation))) {
		// Uninitialized camera positions
		Debug.LogWarning("Uninitialized transforms -> skip alignment");
		return;
	}

	virtualCameraStereo = rightCamera.position - leftCamera.position;
	if (!(IsFinite(virtualCameraStereo.magnitude) &&
		virtualCameraStereo.magnitude > float.Epsilon)) {
		// Unmodified camera positions
		Debug.LogWarning("Bad virtualCameraStereo = " + virtualCameraStereo + " -> skip alignment");
		return;
	}

	if (unlockHold == KeyCode.None ||
		Input.GetKey(unlockHold)) {
		// Manual Time Alignment
		if (Input.GetKeyDown(moreRewind)) {
			rewindAdjust += 0.1f;
		}
		if (Input.GetKeyDown(lessRewind)) {
			rewindAdjust -= 0.1f;
		}
	}

	// IMPORTANT: UpdateHistory must happen first, before any transforms are modified.
	UpdateHistory();

	// IMPORTANT: UpdateAlignment must precede UpdateTimeWarp, since UpdateTimeWarp applies warping relative current positions
	UpdateAlignment();
	UpdateTimeWarp();
}

void ALeapCameraAlignment::UpdateHistory() {
	lastFrame = timeFrame;
	timeFrame = imageRetriever.LeapNow();

	long deltaFrame = timeFrame - lastFrame;
	long deltaImage = timeFrame - imageRetriever.ImageNow();
	if (deltaFrame + deltaImage < maxLatency) {
		frameLatency.Update((float)deltaFrame, Time.deltaTime);
		imageLatency.Update((float)deltaImage, Time.deltaTime);
		//Debug.Log ("Leap deltaTime = " + ((float)deltaTime / 1000f) + " ms");
		//Debug.Log ("Unity deltaTime = " + (Time.deltaTime * 1000f) + " ms");
		// RESULT: Leap & Unity deltaTime measurements are consistent within error tolerance.
		// Leap deltaTime will be used, since it references the same clock as images.
	}
	else {
		// Expect high latency during initial frames
		Debug.LogWarning("Maximum latency exceeded: " + ((float)(deltaFrame + deltaImage) / 1000f) + " ms -> reset latency estimates");
		frameLatency.value = 0f;
		imageLatency.value = 0f;
		frameLatency.reset = true;
		imageLatency.reset = true;
	}

	// Add current position and rotation to history
	history.Add(new TransformData(){
		leapTime = timeFrame,
		position = centerCamera.transform.position,
		rotation = centerCamera.transform.rotation
	});
	//Debug.Log ("Last Index Time = " + history[history.Count-1].leapTime + " =? " + timeFrame);
	// NOTE: history.Add can be retrieved as history[history.Count-1]

	// Reduce history length
	while (history.Count > 0 &&
		maxLatency < timeFrame - history[0].leapTime) {
		//Debug.Log ("Removing oldest from history.Count = " + history.Count);
		history.RemoveAt(0);
	}
}

void ALeapCameraAlignment::UpdateAlignment() {
	long rewindTime = imageRetriever.ImageNow() - (long)frameLatency.value - (long)(rewindAdjust*frameLatency.value);
	long tweenAddition = (long)((1f - tweenRewind) * (float)(timeFrame - rewindTime));
	TransformData past = TransformAtTime(rewindTime + tweenAddition);

	float separate = (tweenPosition * deviceInfo.baseline + (1f - tweenPosition) * virtualCameraStereo.magnitude) * 0.5f;
	float forward = tweenPosition * tweenForward * deviceInfo.focalPlaneOffset;
	Vector3 moveSeparate = past.rotation * Vector3.right * separate;
	Vector3 moveForward = past.rotation * Vector3.forward * forward;

	// Move Virtual cameras to align position & orientation
	centerCamera.transform.position = past.position + moveForward;
	centerCamera.transform.rotation = past.rotation;
	rightCamera.position = past.position + moveForward + moveSeparate;
	rightCamera.rotation = past.rotation;
	leftCamera.position = past.position + moveForward - moveSeparate;
	leftCamera.rotation = past.rotation;
}

void ALeapCameraAlignment::UpdateTimeWarp() {
	long rewindTime = imageRetriever.ImageNow() - (long)(rewindAdjust*frameLatency.value);
	long tweenAddition = (long)((1f - tweenTimeWarp) * (float)(timeFrame - rewindTime));
	TransformData past = TransformAtTime(rewindTime + tweenAddition);

	// Apply only a rotation ~ assume all objects are infinitely distant
	Quaternion rotateImageToNow = centerCamera.transform.rotation * Quaternion.Inverse(past.rotation);
	Matrix4x4 ImageToNow = Matrix4x4.TRS(Vector3.zero, centerCamera.transform.rotation * Quaternion.Inverse(past.rotation), Vector3.one);

	foreach(LeapImageBasedMaterial image in warpedImages) {
		image.GetComponent<Renderer>().material.SetMatrix("_ViewerImageToNow", ImageToNow);
	}
	centerCamera.localRotation = Quaternion.Inverse(rotateImageToNow) * centerCamera.localRotation;
}

bool ALeapCameraAlignment::IsFinite(float f) {
	return !(float.IsInfinity(f) || float.IsNaN(f));
}

bool ALeapCameraAlignment::IsFinite(Vector3 v) {
	return IsFinite(v.x) && IsFinite(v.y) && IsFinite(v.z);
}

bool ALeapCameraAlignment::IsFinite(Quaternion q) {
	return IsFinite(q.w) && IsFinite(q.x) && IsFinite(q.y) && IsFinite(q.z);
}

bool ALeapCameraAlignment::IsFinite(TransformData t) {
	return IsFinite(t.position) && IsFinite(t.rotation);
}

