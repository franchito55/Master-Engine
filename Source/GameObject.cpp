#include "Globals.h"
#include "GameObject.h"
#include "Mesh.h"
#include "Material.h"
#include "Application.h"
#include "ModuleCameraEditor.h"

extern Application* app;

GameObject::GameObject() {
	mesh = new Mesh();
	material = new Material();
	model = Matrix::CreateScale(transform.scale) *
		Matrix::CreateFromQuaternion(transform.rotation) *
		Matrix::CreateTranslation(transform.position);

	aabb = {
		transform.position + Vector3(0.95f, 1.625f, -0.6f),
		transform.position + Vector3(0.95f, 1.625f, 0.55f),
		transform.position + Vector3(0.95f, 0.0f, -0.6f),
		transform.position + Vector3(0.95f, 0.0f, 0.55f),
		transform.position + Vector3(-0.7f, 1.625f, -0.6f),
		transform.position + Vector3(-0.7f, 1.625f, 0.55f),
		transform.position + Vector3(-0.7f, 0.0f, -0.6f),
		transform.position + Vector3(-0.7f, 0.0f, 0.55f),
	};
}

GameObject::~GameObject() {
	// What to do? Multiple GameObjects might be pointing to same mesh/texture
	// delete mesh;
	// delete texture;
}

void GameObject::render(ComPtr<ID3D12GraphicsCommandList> commandList, D3D12_VERTEX_BUFFER_VIEW& vBV, D3D12_INDEX_BUFFER_VIEW& iBV) {
	setModelMatrix(Matrix::CreateScale(getTransform().scale) *
		Matrix::CreateFromQuaternion(getTransform().rotation) *
		Matrix::CreateTranslation(getTransform().position));
	setMvpMatrix((getModelMatrix() * app->getModuleCameraEditor().getViewMatrix() * app->getModuleCameraEditor().getProjectionMatrix()).Transpose());

	// Set cbuffers
	getMvpData()->mvp = getMvpMatrix();
	commandList->SetGraphicsRootConstantBufferView(0, getMvpCB()->GetGPUVirtualAddress());

	// Need to transpose model for row-major in GPU
	getModelMatrixData()->modelMatrix = getModelMatrix().Transpose();
	commandList->SetGraphicsRootConstantBufferView(1, getModelCB()->GetGPUVirtualAddress());

	// Normal matrix = Transpose of inverse of model matrix
	Matrix invTransModel = getModelMatrix().Invert().Transpose();
	getNormalData()->normalMatrix = {
		invTransModel._11, invTransModel._12, invTransModel._13,
		invTransModel._21, invTransModel._22, invTransModel._23,
		invTransModel._31, invTransModel._32, invTransModel._33,
	};
	commandList->SetGraphicsRootConstantBufferView(2, getNormalCB()->GetGPUVirtualAddress());

	commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set current index and vertex buffers
	commandList->IASetIndexBuffer(&iBV);
	commandList->IASetVertexBuffers(0, 1, &vBV);

	commandList->DrawIndexedInstanced(getMesh().getNumIndices(), 1, 0, 0, 0);

	aabb = {
		transform.position + Vector3(0.95f, 1.625f, -0.6f),
		transform.position + Vector3(0.95f, 1.625f, 0.55f),
		transform.position + Vector3(0.95f, 0.0f, -0.6f),
		transform.position + Vector3(0.95f, 0.0f, 0.55f),
		transform.position + Vector3(-0.7f, 1.625f, -0.6f),
		transform.position + Vector3(-0.7f, 1.625f, 0.55f),
		transform.position + Vector3(-0.7f, 0.0f, -0.6f),
		transform.position + Vector3(-0.7f, 0.0f, 0.55f),
	};

	float color[3] = { 0.5f, 0.5f, 0.5f };
	dd::line(&aabb.points[0].x, &aabb.points[1].x, color);
	dd::line(&aabb.points[1].x, &aabb.points[3].x, color);
	dd::line(&aabb.points[3].x, &aabb.points[2].x, color);
	dd::line(&aabb.points[2].x, &aabb.points[0].x, color);
	dd::line(&aabb.points[4].x, &aabb.points[5].x, color);
	dd::line(&aabb.points[5].x, &aabb.points[7].x, color);
	dd::line(&aabb.points[7].x, &aabb.points[6].x, color);
	dd::line(&aabb.points[6].x, &aabb.points[4].x, color);
	dd::line(&aabb.points[0].x, &aabb.points[4].x, color);
	dd::line(&aabb.points[1].x, &aabb.points[5].x, color);
	dd::line(&aabb.points[2].x, &aabb.points[6].x, color);
	dd::line(&aabb.points[3].x, &aabb.points[7].x, color);
	dd::line(&aabb.points[0].x, &aabb.points[4].x, color);
	dd::line(&aabb.points[1].x, &aabb.points[5].x, color);
	dd::line(&aabb.points[2].x, &aabb.points[6].x, color);
	dd::line(&aabb.points[3].x, &aabb.points[7].x, color);
}