#include "Globals.h"
#include "CameraComponent.h"
#include "Application.h"
#include "ModuleCameraEditor.h"
#define _USE_MATH_DEFINES
#include "math.h"

extern Application* app;

CameraComponent::CameraComponent() {
	transform.position = { 0.0f, 1.0f, 5.0f };
	transform.rotation = Quaternion::CreateFromYawPitchRoll(-180.0f * (M_PI / 180.0f), 0.0f * (M_PI / 180.0f), 0.0f * (M_PI / 180.0f));
	model = Matrix::CreateFromQuaternion(transform.rotation) *
		Matrix::CreateTranslation(transform.position);
	recalculateFrustum();
}

void CameraComponent::update() {

}

void CameraComponent::render(ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_VERTEX_BUFFER_VIEW& vBV, D3D12_INDEX_BUFFER_VIEW& iBV) {
	Matrix world =
		Matrix::CreateFromQuaternion(transform.rotation) *
		Matrix::CreateTranslation(transform.position);

	float color[3] = { 0.3f, 0.3f, 0.3f };

	Vector3 frustumPoints[8] = {};
	calculateFrustumVerticesFromFrustum(frustumPoints);

	float color0[3] = { 0.0f, 0.0f, 0.0f };
	dd::sphere(&frustumPoints[0].x, color0, 0.05f);
	float color1[3] = { 0.0f, 0.0f, 1.0f };
	dd::sphere(&frustumPoints[1].x, color1, 0.05f);
	float color2[3] = { 0.0f, 1.0f, 0.0f };
	dd::sphere(&frustumPoints[2].x, color2, 0.05f);
	float color3[3] = { 0.0f, 1.0f, 1.0f };
	dd::sphere(&frustumPoints[3].x, color3, 0.05f);
	float color4[3] = { 1.0f, 0.0f, 0.0f};
	dd::sphere(&frustumPoints[4].x, color4, 0.05f);
	float color5[3] = { 1.0f, 0.0f, 1.0f };
	dd::sphere(&frustumPoints[5].x, color5, 0.05f);
	float color6[3] = { 1.0f, 1.0f, 0.0f };
	dd::sphere(&frustumPoints[6].x, color6, 0.05f);
	float color7[3] = { 1.0f, 1.0f, 1.0f };
	dd::sphere(&frustumPoints[7].x, color7, 0.05f);

	Vector3 frontFaceNormal = frustum.frontFace.Normal();
	Vector3 frontFaceCenter = transform.position + transform.forward * nearPlane;
	dd::plane(&frontFaceCenter.x, &frontFaceNormal.x, color0, color0, 1.0f, 1.0f);

	Vector3 backFaceNormal = frustum.backFace.Normal();
	Vector3 backFaceCenter = transform.position + transform.forward * farPlane;
	dd::plane(&backFaceCenter.x, &backFaceNormal.x, color1, color1, 1.0f, 1.0f);

	Vector3 topFaceNormal = frustum.topFace.Normal();
	Vector3 topFaceCenter = transform.position + transform.forward * nearPlane + transform.forward * ((farPlane - nearPlane) / 2);
	topFaceCenter.y = frustumPoints[0].y + (frustumPoints[4].y - frustumPoints[0].y) / 2;
	dd::plane(&topFaceCenter.x, &topFaceNormal.x, color2, color2, 1.0f, 1.0f);

	Vector3 bottomFaceNormal = frustum.bottomFace.Normal();
	Vector3 bottomFaceCenter = transform.position + transform.forward * nearPlane + transform.forward * ((farPlane - nearPlane) / 2);
	bottomFaceCenter.y = frustumPoints[2].y + (frustumPoints[6].y - frustumPoints[2].y) / 2;
	dd::plane(&bottomFaceCenter.x, &bottomFaceNormal.x, color3, color3, 1.0f, 1.0f);

	Vector3 leftFaceNormal = frustum.leftFace.Normal();
	Vector3 leftFaceCenter = transform.position + transform.forward * nearPlane + transform.forward * ((farPlane - nearPlane) / 2);
	leftFaceCenter.x = frustumPoints[0].x - (frustumPoints[4].x - frustumPoints[0].x) / 2;
	dd::plane(&leftFaceCenter.x, &leftFaceNormal.x, color4, color4, 1.0f, 1.0f);

	Vector3 rightFaceNormal = frustum.rightFace.Normal();
	Vector3 rightFaceCenter = transform.position + transform.forward * nearPlane + transform.forward * ((farPlane - nearPlane) / 2);
	rightFaceCenter.x = frustumPoints[1].x + (frustumPoints[7].x - frustumPoints[1].x) / 2;
	dd::plane(&rightFaceCenter.x, &rightFaceNormal.x, color5, color5, 1.0f, 1.0f);

	dd::line(&frustumPoints[0].x, &frustumPoints[1].x, color);
	dd::line(&frustumPoints[1].x, &frustumPoints[2].x, color);
	dd::line(&frustumPoints[2].x, &frustumPoints[3].x, color);
	dd::line(&frustumPoints[3].x, &frustumPoints[0].x, color);
	dd::line(&frustumPoints[4].x, &frustumPoints[5].x, color);
	dd::line(&frustumPoints[5].x, &frustumPoints[6].x, color);
	dd::line(&frustumPoints[6].x, &frustumPoints[7].x, color);
	dd::line(&frustumPoints[7].x, &frustumPoints[4].x, color);
	dd::line(&frustumPoints[0].x, &frustumPoints[4].x, color);
	dd::line(&frustumPoints[1].x, &frustumPoints[5].x, color);
	dd::line(&frustumPoints[2].x, &frustumPoints[6].x, color);
	dd::line(&frustumPoints[3].x, &frustumPoints[7].x, color);
}

void CameraComponent::recalculateFrustum() {
	Matrix world =
		Matrix::CreateFromQuaternion(transform.rotation) *
		Matrix::CreateTranslation(transform.position);

	float aspect = 16.0f / 9.0f;

	float tanFov = tan(fov * (M_PI / 180.0f) * 0.5f);

	Vector3 frustumPoints[8] = {};
	calculateFrustumVerticesFromFrustum(frustumPoints);

	frustum.frontFace = Plane(frustumPoints[0], frustumPoints[1], frustumPoints[2]);

	frustum.backFace = Plane(frustumPoints[6], frustumPoints[5], frustumPoints[4]);

	frustum.topFace = Plane(frustumPoints[5], frustumPoints[1], frustumPoints[0]);

	frustum.bottomFace = Plane(frustumPoints[2], frustumPoints[6], frustumPoints[7]);

	frustum.leftFace = Plane(frustumPoints[5], frustumPoints[6], frustumPoints[2]);

	frustum.rightFace = Plane(frustumPoints[0], frustumPoints[3], frustumPoints[7]);
}

Vector3* CameraComponent::calculateFrustumVerticesFromFrustum(Vector3 verts[8]) {
	Matrix world =
		Matrix::CreateFromQuaternion(transform.rotation) *
		Matrix::CreateTranslation(transform.position);

	float aspect = 16.0f / 9.0f;

	float tanFov = tan(fov / aspect * (M_PI / 180.0f) * 0.5f);

	// Near plane
	float nh = nearPlane * tanFov;
	float nw = nh * aspect;

	// Far plane
	float fh = farPlane * tanFov;
	float fw = fh * aspect;

	// Corners in camera space
	Vector3 points[8] = {
		{-nw,  nh, nearPlane}, // top left
		{ nw,  nh, nearPlane}, // top right
		{ nw, -nh, nearPlane}, // bottom right
		{-nw, -nh, nearPlane}, // bottom left
		{-fw,  fh, farPlane}, // top left
		{ fw,  fh, farPlane}, // top right
		{ fw, -fh, farPlane}, // bottom right
		{-fw, -fh, farPlane} // bottom left
	};

	// Transform to world space
	for (auto& v : points) v = Vector3::Transform(v, world);

	for (unsigned int i = 0; i < 8; i++) {
		verts[i] = points[i];
	}
	return verts;
}